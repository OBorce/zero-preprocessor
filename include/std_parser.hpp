#ifndef STD_PARSER_H
#define STD_PARSER_H

#include <any>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <variant>

#include <detect.hpp>
#include <overloaded.hpp>
#include <result.hpp>
#include <std_rules.hpp>
#include <string_utils.hpp>

namespace std_parser {
using CodeFragment =
    std::variant<rules::ast::Namespace, rules::ast::Scope, rules::ast::Class,
                 rules::ast::Enumeration, rules::ast::Function,
                 rules::ast::FunctionDeclaration, rules::ast::Params,
                 rules::ast::Vars, rules::ast::Expression, rules::ast::Lambda,
                 rules::ast::RoundExpression, rules::ast::CurlyExpression,
                 rules::ast::Statement, rules::ast::ReturnStatement,
                 rules::ast::VarStatement, rules::ast::IfExpression>;

class StdParserState {
  class {
    std::vector<CodeFragment> code_fragments = {rules::ast::Namespace{""}};
    rules::ast::SourceLocation current_location = {0, 0};

   public:
    void set_location(rules::ast::SourceLocation l) { current_location = l; }

    auto size() const { return code_fragments.size(); }

    auto& get_code_fragments() { return code_fragments; }

    auto const& get_code_fragments() const { return code_fragments; }

    // TODO: maybe do a real emplace_back but watch out for implicit conversions
    void emplace_back(CodeFragment&& cf) {
      std::visit([this](auto& f) { f.loc = current_location; }, cf);
      code_fragments.emplace_back(std::move(cf));
    }

    void push_back(CodeFragment&& cf) {
      std::visit([this](auto& f) { f.loc = current_location; }, cf);
      code_fragments.emplace_back(std::move(cf));
    }

    void pop_back() { code_fragments.pop_back(); }

    auto& front() { return code_fragments.front(); }

    auto& back() { return code_fragments.back(); }

    auto get_location() const { return current_location; }

    auto& operator[](std::size_t i) { return code_fragments[i]; }
  } ast_state;

  // TODO: includes should be merged with CodeFragment
  std::unordered_set<std::string> includes;

  // some iterator pointing to the start of the function's body
  // FIXME: used to capture a function's body used by the metaclasses
  // until we can recreate the body from our AST
  std::vector<std::any> function_begins;

  template <class Source>
  using ParseResult = PResult<Iter<Source>, std::string_view>;

  template <class Source, class It>
  ParseResult<Source> makeResult(Source source, bool parsed, It begin) {
    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  };
  // parser methods

  template <class Source>
  ParseResult<Source> parse_inside_namespace(Source& source,
                                             rules::ast::Namespace& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(std::move(rez));
    };

    auto fun_decl = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(std::move(rez));
      ast_state.emplace_back(rules::ast::Params{});
    };

    auto fun = [this, &begin](auto& ctx) {
      auto& rez = _attr(ctx);
      // TODO: we can get begin from the context
      function_begins.push_back(begin);
      ast_state.emplace_back(std::move(rez));
    };

    auto var = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(rules::ast::VarStatement{});
      ast_state.emplace_back(rules::ast::Vars{std::move(rez)});
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto sb = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(rules::ast::Namespace{std::move(rez)});
    };

    auto se = [&](auto&) {
      if (ast_state.size() < 2) {
        // NOTE: more closing brackets than opened
        throw std::runtime_error("extraneous closing brace ('}')");
      }

      auto& v = ast_state[ast_state.size() - 2];
      std::visit(overloaded{
                     [&](rules::ast::Namespace& arg) {
                       arg.add_namespace(std::move(current));
                     },
                     [](auto&) {
                       /* NOTE: Can't have a namespace inside a class, function
                        * or a local scope*/
                     },
                 },
                 v);
      ast_state.pop_back();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(
        begin, end,
        // rules begin
        rules::some_space |
            ((rules::class_or_struct >> rules::scope_begin)[nest] |
             (rules::class_or_struct >> rules::statement_end) |
             (rules::function_declaration)[fun_decl] |
             // TODO: change operator_signiture as as function_declaration
             (rules::operator_signiture >> rules::scope_begin)[fun] |
             (rules::operator_signiture >> rules::statement_end) |
             (rules::enumeration >> rules::scope_begin)[nest] |
             (rules::enumeration >> rules::statement_end) |
             rules::namespace_begin[sb] | rules::scope_end[se] |
             rules::include[inc] | rules::comment | rules::param[var])
        // rules end
    );

    // TODO: error msg
    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_class(Source& source,
                                         rules::ast::Class& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(std::move(rez));
    };

    auto fun = [this, &begin](auto& ctx) {
      auto& rez = _attr(ctx);
      // TODO: we can get begin from the context
      function_begins.push_back(begin);
      ast_state.emplace_back(std::move(rez));
    };

    auto funSig = [&](auto& ctx) {
      auto& rez = _attr(ctx);
      rez.loc = ast_state.get_location();
      current.add_function(std::move(rez));
    };

    auto ac = [&](auto& ctx) {
      auto& rez = _attr(ctx);
      current.set_access_modifier(rez);
    };

    auto var = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(rules::ast::VarStatement{});
      ast_state.emplace_back(rules::ast::Vars{std::move(rez)});
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto se = [&](auto&) { this->close_code_fragment<rules::ast::Class>(); };

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(
        begin, end,
        // rules begin
        rules::some_space |
            ((rules::class_or_struct >> rules::scope_begin)[nest] |
             (rules::class_or_struct >> rules::statement_end) |
             (rules::method_signiture >> rules::scope_begin)[fun] |
             (rules::method_signiture >> rules::statement_end)[funSig] |
             (rules::operator_signiture >> rules::scope_begin)[fun] |
             (rules::operator_signiture >> rules::statement_end)[funSig] |
             (rules::constructor >> rules::scope_begin)[fun] |
             (rules::constructor >> rules::statement_end)[funSig] |
             (rules::enumeration >> rules::scope_begin)[nest] |
             (rules::enumeration >> rules::statement_end) |
             rules::scope_end[se] | rules::include[inc] | rules::comment |
             rules::class_access_modifier[ac] | rules::param[var])
        // rules end
    );

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_enum(Source& source,
                                        rules::ast::Enumeration& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto en = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.set_enumerators(std::move(rez));
    };

    auto se = [&](auto&) {
      this->close_code_fragment<rules::ast::Enumeration>();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(
        begin, end,
        // rules begin
        rules::optionaly_space >>
            (rules::enumerators[en] | rules::scope_end[se] | rules::comment)
        // rules end
    );

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_expression(Source& source,
                                              rules::ast::Expression& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto exp = [this, &current](auto& ctx) {
      auto& rez = _attr(ctx);
      std::visit(overloaded{[&current](rules::ast::VariableExpression& e) {
                              current.expressions.emplace_back(std::move(e));
                            },
                            [&current](rules::ast::LiteralExpression& e) {
                              current.expressions.emplace_back(std::move(e));
                            },
                            [&](auto& state) {
                              ast_state.emplace_back(std::move(state));
                            }},
                 rez);
    };

    auto beg = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.operators.emplace_back(std::move(rez));
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;
    if (current.is_begin()) {
      parsed = x3::parse(
          begin, end,
          // rules begin
          rules::optionaly_space >> (rules::comment | rules::expression[exp])
          // rules end
      );
    } else {
      parsed = x3::parse(
          begin, end,
          // rules begin
          rules::optionaly_space >> (rules::comment | rules::operator_sep[beg])
          // rules end
      );
    }

    if (!parsed && !current.is_begin()) {
      close_code_fragment<rules::ast::Expression>();
      return Complate{};
    }

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_round_expression(
      Source& source, rules::ast::RoundExpression& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto se = [this](auto&) {
      close_code_fragment<rules::ast::RoundExpression>();
    };

    auto exp = [this, &current](auto& ctx) {
      auto& rez = _attr(ctx);
      std::visit(overloaded{[&current](rules::ast::VariableExpression& e) {
                              current.expressions.emplace_back(std::move(e));
                            },
                            [&current](rules::ast::LiteralExpression& e) {
                              current.expressions.emplace_back(std::move(e));
                            },
                            [&](auto& state) {
                              ast_state.emplace_back(std::move(state));
                            }},
                 rez);
    };

    auto beg = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.operators.emplace_back(std::move(rez));
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;
    if (current.is_begin()) {
      parsed = x3::parse(begin, end,
                         // rules begin
                         rules::optionaly_space >>
                             (rules::comment | rules::parenthesis_end[se] |
                              rules::expression[exp])
                         // rules end
      );
    } else {
      parsed = x3::parse(begin, end,
                         // rules begin
                         rules::optionaly_space >>
                             (rules::comment | rules::parenthesis_end[se] |
                              rules::operator_sep[beg])
                         // rules end
      );
    }

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_curly_expression(
      Source& source, rules::ast::CurlyExpression& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto se = [this](auto&) {
      close_code_fragment<rules::ast::CurlyExpression>();
    };

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(std::move(rez));
    };

    auto exp = [this, &current](auto& ctx) {
      auto& rez = _attr(ctx);
      std::visit(overloaded{[&current](rules::ast::VariableExpression& e) {
                              current.expressions.emplace_back(std::move(e));
                            },
                            [&current](rules::ast::LiteralExpression& e) {
                              current.expressions.emplace_back(std::move(e));
                            },
                            [&](auto& state) {
                              ast_state.emplace_back(std::move(state));
                            }},
                 rez);
    };

    auto beg = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.operators.emplace_back(std::move(rez));
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;
    if (current.is_begin()) {
      parsed = x3::parse(begin, end,
                         // rules begin
                         rules::optionaly_space >>
                             (rules::comment | rules::curly_begin[nest] |
                              rules::curly_end[se] | rules::expression[exp])
                         // rules end
      );
    } else {
      parsed = x3::parse(
          begin, end,
          // rules begin
          rules::optionaly_space >>
              (rules::comment | rules::curly_end[se] | rules::operator_sep[beg])
          // rules end
      );
    }

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_lambda(Source& source, rules::ast::Lambda&) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(std::move(rez));
    };

    auto stm = [this](auto& ctx) {
      ast_state.emplace_back(rules::ast::Statement{});
      auto& rez = _attr(ctx);
      std::visit([&](auto& state) { ast_state.emplace_back(std::move(state)); },
                 rez);
    };

    auto rstm = [this](auto& ctx) {
      ast_state.emplace_back(rules::ast::ReturnStatement{});
      auto& rez = _attr(ctx);
      std::visit([&](auto& state) { ast_state.emplace_back(std::move(state)); },
                 rez);
    };

    auto var = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(rules::ast::VarStatement{});
      ast_state.emplace_back(rules::ast::Vars{std::move(rez)});
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto sb = [&](auto&) { ast_state.emplace_back(rules::ast::Scope{}); };

    auto se = [this](auto&) { close_code_fragment<rules::ast::Lambda>(); };

    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::some_space |
                      ((rules::class_or_struct >> rules::scope_begin)[nest] |
                       (rules::class_or_struct >> rules::statement_end) |
                       (rules::enumeration >> rules::scope_begin)[nest] |
                       (rules::enumeration >> rules::statement_end) |
                       rules::scope_begin[sb] |
                       (rules::scope_end[se] >> rules::optionaly_space >>
                        -(rules::parenthesis_begin[nest])) |
                       rules::include[inc] | rules::comment |
                       rules::return_statement[rstm] | rules::param[var] |
                       rules::for_loop | rules::if_expression[nest] |
                       rules::else_expression | rules::while_loop |
                       rules::expression[stm])
                  // rules end
        );

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_if_expression(
      Source& source, rules::ast::IfExpression& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto exp = [this, &current](auto&) {
      current.state = rules::ast::IfExpressionState::Done;
      ast_state.emplace_back(rules::ast::Expression{});
    };

    auto close = [this](auto&) {
      close_code_fragment<rules::ast::IfExpression>();
    };

    auto var = [this, &current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.state = rules::ast::IfExpressionState::Expression;
      ast_state.emplace_back(rules::ast::Vars{std::move(rez)});
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;
    switch (current.state) {
      case rules::ast::IfExpressionState::Begin:
        parsed = x3::parse(begin, end,
                           // rules begin
                           '(' >> rules::optionaly_space >>
                               (rules::comment | rules::var_with_init[var] |
                                &rules::expression[exp])
                           // rules end
        );
        break;
      case rules::ast::IfExpressionState::Expression:
        parsed = x3::parse(
            begin, end,
            // rules begin
            rules::optionaly_space >>
                ((';' >> rules::optionaly_space >> &rules::expression)[exp] |
                 rules::parenthesis_end[close])
            // rules end
        );
        break;
      case rules::ast::IfExpressionState::Done:
        parsed =
            x3::parse(begin, end,
                      // rules begin
                      rules::optionaly_space >> rules::parenthesis_end[close]
                      // rules end
            );
        break;
    }

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source, class Statement>
  ParseResult<Source> parse_inside_statement(Source& source, Statement&) {
    auto begin = source.begin();
    auto end = source.end();

    auto se = [this](auto&) { close_code_fragment<Statement>(); };

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(begin, end,
                            // rules begin
                            rules::optionaly_space >> rules::statement_end[se]
                            // rules end
    );

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_var_definition(Source& source,
                                                  rules::ast::Vars& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto exp = [this, &current](auto&) {
      current.state = rules::ast::VarDefinition::Next;
      ast_state.emplace_back(rules::ast::Expression{});
    };

    auto init = [this, &current](auto&) {
      current.state = rules::ast::VarDefinition::Next;
      ast_state.emplace_back(rules::ast::CurlyExpression{});
    };

    auto var = [this, &current](auto& ctx) {
      auto& rez = _attr(ctx);
      auto loc = ast_state.get_location();
      current.add_var(std::move(rez), loc);
      current.state = rules::ast::VarDefinition::Init;
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;

    switch (current.state) {
      case rules::ast::VarDefinition::Init:
        parsed = x3::parse(
            begin, end,
            // rules begin
            rules::optionaly_space >>
                (rules::comment |
                 (('=' >> rules::optionaly_space >> &rules::expression)[exp] |
                  (-('=' >> rules::optionaly_space) >>
                   rules::curly_begin)[init] |
                  (',' >> rules::optionaly_space >> rules::name)[var]))
            // rules end
        );
        break;
      case rules::ast::VarDefinition::Next:
        parsed =
            x3::parse(begin, end,
                      // rules begin
                      rules::optionaly_space >>
                          (rules::comment |
                           (',' >> rules::optionaly_space >> rules::name)[var])
                      // rules end
            );
        break;
    }

    if (!parsed) {
      close_code_fragment<rules::ast::Vars>();
      return Complate{};
    }

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_function_declaration(
      Source& source, rules::ast::FunctionDeclaration& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto en = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.is_noexcept = rez;
    };

    auto se = [&](auto&) {
      this->close_code_fragment<rules::ast::FunctionDeclaration>();
    };

    auto sb = [&](auto&) {
      rules::ast::Function fun = std::move(current);
      ast_state.pop_back();
      ast_state.emplace_back(std::move(fun));
      function_begins.push_back(begin);
    };

    // TODO: not finished
    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::optionaly_space >> ')' >> rules::optionaly_space >>
                      -rules::is_noexcept[en] >>
                      (rules::statement_end[se] | rules::scope_begin[sb])
                  // rules end
        );

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_params(Source& source,
                                          rules::ast::Params& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto exp = [this, &current](auto& ctx) {
      auto& rez = _attr(ctx);
      auto& last = current.parameters.back().init;
      std::visit(overloaded{[&last](rules::ast::VariableExpression& e) {
                              last.expressions.emplace_back(std::move(e));
                            },
                            [&last](rules::ast::LiteralExpression& e) {
                              last.expressions.emplace_back(std::move(e));
                            },
                            [&](auto& state) {
                              ast_state.emplace_back(std::move(state));
                            }},
                 rez);
    };

    auto beg = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      auto& last = current.parameters.back().init;
      last.operators.emplace_back(std::move(rez));
    };

    auto init = [this, &current](auto&) {
      ast_state.emplace_back(rules::ast::CurlyExpression{});
    };

    auto var = [this, &current](auto& ctx) {
      auto& rez = _attr(ctx);
      auto loc = ast_state.get_location();
      rez.loc = loc;
      current.parameters.push_back(std::move(rez));
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;
    bool first = current.parameters.empty();

    if (first) {
      parsed = x3::parse(begin, end,
                         // rules begin
                         rules::optionaly_space >>
                             (rules::comment | (rules::optional_param[var]))
                         // rules end
      );
    } else {
      auto& last = current.parameters.back().init;
      if (last.empty()) {
        parsed = x3::parse(
            begin, end,
            // rules begin
            rules::optionaly_space >>
                (rules::comment |
                 (',' >> rules::optionaly_space >> rules::optional_param)[var] |
                 // TODO: try to join the two into one
                 (('=' >> rules::optionaly_space >> rules::expression)[exp] |
                  ('=' >> rules::optionaly_space >> rules::curly_begin)[init]))
            // rules end
        );
      } else if (last.is_begin()) {
        parsed = x3::parse(
            begin, end,
            // rules begin
            rules::optionaly_space >> (rules::comment | rules::expression[exp])
            // rules end
        );
      } else {
        parsed = x3::parse(
            begin, end,
            // rules begin
            rules::optionaly_space >>
                (rules::comment | rules::param_operator_sep[beg] |
                 (',' >> rules::optionaly_space >> rules::optional_param)[var])
            // rules end
        );
      }
    }

    if (!parsed) {
      if (not first) {
        auto& last = current.parameters.back().init;
        if (not last.empty() and last.is_begin()) {
          // not finished init expression left with a binary_operator
          return Error{};
        }
      }

      close_code_fragment<rules::ast::Params>();
      // TODO: maybe add optional error message of what was expected next for a
      // successful parse
      return Complate{};
    } else {
      return Result{begin, make_string_view(source.begin(), begin)};
    }
  }

  template <class Source>
  ParseResult<Source> parse_inside_function(Source& source,
                                            rules::ast::Function& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(std::move(rez));
    };

    auto stm = [this](auto& ctx) {
      ast_state.emplace_back(rules::ast::Statement{});
      auto& rez = _attr(ctx);
      std::visit([&](auto& state) { ast_state.emplace_back(std::move(state)); },
                 rez);
    };

    auto rstm = [this](auto& ctx) {
      ast_state.emplace_back(rules::ast::ReturnStatement{});
      auto& rez = _attr(ctx);
      std::visit([&](auto& state) { ast_state.emplace_back(std::move(state)); },
                 rez);
    };

    auto var = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(rules::ast::VarStatement{});
      ast_state.emplace_back(rules::ast::Vars{std::move(rez)});
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto sb = [&](auto&) { ast_state.emplace_back(rules::ast::Scope{}); };

    auto se = [&](auto&) {
      // TODO: remove function_begins when not needed
      if (ast_state.size() < 2 || function_begins.empty()) {
        // NOTE: more closing brackets than opened
        throw std::runtime_error("extraneous closing brace ('}')");
      }
      auto current_functions_start = function_begins.back();
      auto end = source.begin();
      auto start = std::any_cast<decltype(end)>(current_functions_start);
      current.body = std::string(start, end) + '\n';
      auto& v = ast_state[ast_state.size() - 2];
      std::visit(overloaded{
                     [&](rules::ast::Namespace& arg) {
                       arg.add_function(std::move(current));
                     },
                     [&](rules::ast::Class& arg) {
                       arg.add_function(std::move(current));
                     },
                     [](auto&) {
                       /* NOTE: Can't have a function inside a function or local
                        * scope*/
                     },
                 },
                 v);
      ast_state.pop_back();
      function_begins.pop_back();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::some_space |
                      ((rules::class_or_struct >> rules::scope_begin)[nest] |
                       (rules::class_or_struct >> rules::statement_end) |
                       (rules::enumeration >> rules::scope_begin)[nest] |
                       (rules::enumeration >> rules::statement_end) |
                       rules::scope_begin[sb] | rules::scope_end[se] |
                       rules::include[inc] | rules::comment |
                       rules::return_statement[rstm] | rules::param[var] |
                       rules::for_loop | rules::if_expression[nest] |
                       rules::else_expression | rules::while_loop |
                       rules::expression[stm])
                  // rules end
        );

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  template <class Source>
  ParseResult<Source> parse_inside_scope(Source& source, rules::ast::Scope&) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(std::move(rez));
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto var = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      ast_state.emplace_back(rules::ast::VarStatement{});
      ast_state.emplace_back(rules::ast::Vars{std::move(rez)});
    };

    auto stm = [this](auto& ctx) {
      ast_state.emplace_back(rules::ast::Statement{});
      auto& rez = _attr(ctx);
      std::visit([&](auto& state) { ast_state.emplace_back(std::move(state)); },
                 rez);
    };

    auto rstm = [this](auto& ctx) {
      ast_state.emplace_back(rules::ast::ReturnStatement{});
      auto& rez = _attr(ctx);
      std::visit([&](auto& state) { ast_state.emplace_back(std::move(state)); },
                 rez);
    };

    auto sb = [&](auto&) { ast_state.emplace_back(rules::ast::Scope{}); };

    auto se = [&](auto&) { close_code_fragment<rules::ast::Scope>(); };

    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::some_space |
                      ((rules::class_or_struct >> rules::scope_begin)[nest] |
                       (rules::class_or_struct >> rules::statement_end) |
                       (rules::enumeration >> rules::scope_begin)[nest] |
                       (rules::enumeration >> rules::statement_end) |
                       rules::scope_begin[sb] | rules::scope_end[se] |
                       rules::include[inc] | rules::comment |
                       rules::return_statement[rstm] | rules::param[var] |
                       rules::for_loop | rules::if_expression[nest] |
                       rules::else_expression | rules::while_loop |
                       rules::expression[stm])
                  // rules end
        );

    if (parsed) {
      return Result{begin, make_string_view(source.begin(), begin)};
    }

    return Error{};
  }

  void close_current_class() {
    auto& c = std::get<rules::ast::Class>(ast_state.back());
    auto& v = ast_state[ast_state.size() - 2];
    std::visit(
        overloaded{
            [&](rules::ast::Namespace& arg) { arg.add_class(std::move(c)); },
            [&](rules::ast::Class& arg) { arg.add_class(std::move(c)); },
            [](auto&) {
              /* TODO: local classes inside functions or local scopes*/
            },
        },
        v);
  }

  void close_current_enum() {
    auto& c = std::get<rules::ast::Enumeration>(ast_state.back());
    auto& v = ast_state[ast_state.size() - 2];
    std::visit(
        overloaded{
            [&](rules::ast::Namespace& arg) { arg.add_enum(std::move(c)); },
            [&](rules::ast::Class& arg) { arg.add_enum(std::move(c)); },
            [](auto&) {
              /* TODO: local enums inside functions or local scopes*/
            },
        },
        v);
  }

  void close_current_var_declaration() {
    auto& c = std::get<rules::ast::Vars>(ast_state.back());
    auto& v = ast_state[ast_state.size() - 2];
    std::visit(overloaded{
                   [&](rules::ast::VarStatement& arg) {
                     arg.variables = std::move(c.variables);
                     arg.loc = std::move(c.loc);
                   },
                   [](auto&) {
                     /* other can't have variables*/
                   },
               },
               v);
  }

  void close_current_scope() {
    auto& c = std::get<rules::ast::Scope>(ast_state.back());
    auto& v = ast_state[ast_state.size() - 2];
    std::visit(overloaded{
                   [&](rules::ast::Function& arg) {
                     arg.statements.emplace_back(std::move(c));
                   },
                   [&](rules::ast::Scope& arg) {
                     arg.statements.emplace_back(std::move(c));
                   },
                   [](auto&) {
                     /* other can't have local scopes*/
                   },
               },
               v);
  }

  template <class Expression>
  void move_expression(CodeFragment& cf, Expression&& e) {
    std::visit(
        overloaded{
            [&](rules::ast::Expression& arg) {
              arg.expressions.emplace_back(std::move(e));
            },
            [&](rules::ast::RoundExpression& arg) {
              arg.expressions.emplace_back(std::move(e));
            },
            [&](rules::ast::CurlyExpression& arg) {
              arg.expressions.emplace_back(std::move(e));
            },
            [&](rules::ast::Statement& arg) {
              arg.expression = rules::ast::Expression(std::move(e));
            },
            [&](rules::ast::ReturnStatement& arg) {
              arg.expression = rules::ast::Expression(std::move(e));
            },
            [&](rules::ast::Vars& arg) {
              if constexpr (std::is_same_v<std::remove_reference_t<Expression>,
                                           rules::ast::CurlyExpression>) {
                arg.variables.back().init =
                    rules::ast::Expression(std::move(e));
              }
            },
            [&](rules::ast::Params& arg) {
              arg.parameters.back().init.expressions.emplace_back(std::move(e));
            },
            [](auto&) {
              /* other can't have round/curly expressions*/
            },
        },
        cf);
  }

  template <class Expression>
  void close_current_expression() {
    auto& expression = std::get<Expression>(ast_state.back());
    auto& variant_code_fragment = ast_state[ast_state.size() - 2];
    move_expression(variant_code_fragment, expression);
  }

  void close_current_expression() {
    auto& expression = std::get<rules::ast::Expression>(ast_state.back());
    auto& variant_code_fragment = ast_state[ast_state.size() - 2];
    std::visit(overloaded{
                   [&](rules::ast::Statement& arg) {
                     arg.expression = std::move(expression);
                   },
                   [&](rules::ast::ReturnStatement& arg) {
                     arg.expression = std::move(expression);
                   },
                   [&](rules::ast::Vars& arg) {
                     arg.variables.back().init = std::move(expression);
                   },
                   [](auto&) {
                     /* other can't have expressions*/
                   },
               },
               variant_code_fragment);
  }

  void close_current_varstatement() {
    auto& vs = std::get<rules::ast::VarStatement>(ast_state.back());
    auto& variant_code_fragment = ast_state[ast_state.size() - 2];
    std::visit(
        overloaded{
            [&](rules::ast::Class& arg) { arg.add_variables(vs.variables); },
            [&](rules::ast::Namespace& arg) {
              arg.add_variables(vs.variables);
            },
            [&](rules::ast::Function& arg) {
              arg.statements.emplace_back(std::move(vs));
            },
            [&](rules::ast::Scope& arg) {
              arg.statements.emplace_back(std::move(vs));
            },
            [](auto&) {
              /* other can't have var statements*/
            },
        },
        variant_code_fragment);
  }

  template <class Statement>
  void close_current_statement() {
    auto& statement = std::get<Statement>(ast_state.back());
    auto& variant_code_fragment = ast_state[ast_state.size() - 2];
    std::visit(overloaded{
                   [&](rules::ast::Function& arg) {
                     arg.statements.emplace_back(std::move(statement));
                   },
                   [&](rules::ast::Scope& arg) {
                     arg.statements.emplace_back(std::move(statement));
                   },
                   [&](rules::ast::Lambda& arg) {
                     arg.statements.emplace_back(std::move(statement));
                   },
                   [](auto&) {
                     /* other can't have statements*/
                   },
               },
               variant_code_fragment);
  }

  template <class T>
  using Iter = decltype(std::declval<T>().begin());

 public:
  /**
   * Parse a single statement from the source
   *
   * Returns an `optional<iterator>` if successfully parsed a statement pointing
   * to the end of the parsed statement in the source and the result to be
   * outputed, else empty
   */
  template <class Source>
  std::optional<Result<Iter<Source>, std::string_view>> parse(Source& source) {
    ast_state.set_location({source.get_row(), source.get_column()});

    ParseResult<Source> rez = Complate{};
    do {
      auto& current_code_fragment = ast_state.back();
      rez = std::visit(overloaded{
                           [&](rules::ast::Namespace& arg) {
                             return parse_inside_namespace(source, arg);
                           },
                           [&](rules::ast::Class& arg) {
                             return parse_inside_class(source, arg);
                           },
                           [&](rules::ast::Function& arg) {
                             return parse_inside_function(source, arg);
                           },
                           [&](rules::ast::FunctionDeclaration& arg) {
                             return parse_inside_function_declaration(source,
                                                                      arg);
                           },
                           [&](rules::ast::Params& arg) {
                             return parse_inside_params(source, arg);
                           },
                           [&](rules::ast::Scope& arg) {
                             return parse_inside_scope(source, arg);
                           },
                           [&](rules::ast::Enumeration& arg) {
                             return parse_inside_enum(source, arg);
                           },
                           [&](rules::ast::Expression& arg) {
                             return parse_inside_expression(source, arg);
                           },
                           [&](rules::ast::RoundExpression& arg) {
                             return parse_inside_round_expression(source, arg);
                           },
                           [&](rules::ast::CurlyExpression& arg) {
                             return parse_inside_curly_expression(source, arg);
                           },
                           [&](rules::ast::Lambda& arg) {
                             return parse_inside_lambda(source, arg);
                           },
                           [&](rules::ast::Statement& arg) {
                             return parse_inside_statement(source, arg);
                           },
                           [&](rules::ast::ReturnStatement& arg) {
                             return parse_inside_statement(source, arg);
                           },
                           [&](rules::ast::VarStatement& arg) {
                             return parse_inside_statement(source, arg);
                           },
                           [&](rules::ast::Vars& arg) {
                             return parse_inside_var_definition(source, arg);
                           },
                           [&](rules::ast::IfExpression& arg) {
                             return parse_inside_if_expression(source, arg);
                           },
                       },
                       current_code_fragment);
    } while (std::holds_alternative<Complate>(rez));

    if (std::holds_alternative<Result<Iter<Source>, std::string_view>>(rez)) {
      return std::get<Result<Iter<Source>, std::string_view>>(rez);
    }

    // TODO: return an error msg
    return {};
  }

  /**
   * Add a new code_fragment
   */
  void open_new_code_fragment(CodeFragment&& arg) {
    // TODO: add source location?? or already has source location set
    ast_state.emplace_back(std::move(arg));
  }

  /**
   * Return a constant view of all the code_fragments
   */
  auto const& get_all_code_fragments() const {
    return ast_state.get_code_fragments();
  }

  CodeFragment& get_current_code_fragment() { return ast_state.back(); }

  rules::ast::Namespace const& get_top_code_fragment() {
    return std::get<rules::ast::Namespace>(ast_state.front());
  }

  // TODO add SFINAE for CodeFragment
  template <class Fragment>
  void close_code_fragment() {
    if (ast_state.size() < 2) {
      // NOTE: more closing brackets than opened
      throw std::runtime_error("extraneous closing brace ('}')");
    }

    if constexpr (std::is_same<Fragment, rules::ast::Class>()) {
      close_current_class();
    } else if constexpr (std::is_same<Fragment, rules::ast::Enumeration>()) {
      close_current_enum();
    } else if constexpr (std::is_same<Fragment, rules::ast::Vars>()) {
      close_current_var_declaration();
    } else if constexpr (std::is_same<Fragment, rules::ast::Scope>()) {
      close_current_scope();
    } else if constexpr (is_one_of<Fragment, rules::ast::RoundExpression,
                                   rules::ast::CurlyExpression,
                                   rules::ast::Lambda>) {
      close_current_expression<Fragment>();
    } else if constexpr (std::is_same<Fragment, rules::ast::Expression>()) {
      close_current_expression();
    } else if constexpr (is_one_of<Fragment, rules::ast::Statement,
                                   rules::ast::ReturnStatement>) {
      close_current_statement<Fragment>();
    } else if constexpr (std::is_same<Fragment, rules::ast::VarStatement>()) {
      close_current_varstatement();
    } else {
      // TODO: implement
    }

    ast_state.pop_back();
  }

  template <class Source>
  auto get_includes(Source& source) {
    auto begin = source.begin();
    auto end = source.end();
    std::unordered_set<std::string> includes;

    auto inc = [&includes](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    while (begin != end) {
      namespace x3 = boost::spirit::x3;
      bool parsed =
          x3::parse(begin, end,
                    // rules begin
                    rules::some_space | rules::include[inc] | rules::skip_line
                    // rules end
          );

      if (!parsed) {
        std::size_t remaining = std::distance(begin, end);
        std::size_t size = std::min<std::size_t>(30ul, remaining);
        std::string_view content = {&*begin, size};

        std::string error_msg = "source can't be parsed by the std parser: ";
        error_msg += std::to_string(content.size());
        error_msg += content;
        throw std::runtime_error(error_msg);
      }
    }

    return std::move(includes);
  }

  /**
   * Return all the includes in the parsed file
   */
  auto& get_all_includes() { return includes; }
};  // namespace std_parser

class StdParser {
  template <class Iter, class Id, class T, bool B,
            template <class, class, bool> class Rule, class Out = T>
  auto try_parse_T(Iter begin, Iter end, Rule<Id, T, B> rule) {
    Out tmp;
    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(begin, end,
                            // rules begin
                            rules::optionaly_space >> rule,
                            // rules end
                            tmp);

    return parsed ? std::optional{ResultT{begin, std::move(tmp)}}
                  : std::nullopt;
  }

  /**
   * Used as a helper when we want to specify a different out type
   */
  template <class Out, class Iter, class Id, class T, bool B,
            template <class, class, bool> class Rule>
  auto try_parse_T(Iter begin, Iter end, Rule<Id, T, B> rule) {
    return try_parse_T<Iter, Id, T, B, Rule, Out>(begin, end, rule);
  }

  StdParserState parser;

 public:
  // TODO: when supported in std=c++2a change to fixed length string
  constexpr static int id = 's' + 't' + 'd';

  template <class Iter>
  auto try_parse_function(Iter begin, Iter end) {
    return try_parse_T<rules::ast::Function>(begin, end, rules::function_start);
  }

  template <class Iter>
  auto try_parse_templates_params(Iter begin, Iter end) {
    return try_parse_T(begin, end, rules::template_parameters);
  }

  template <class Iter>
  auto try_parse_name(Iter begin, Iter end) {
    return try_parse_T(begin, end, rules::name);
  }

  template <class Iter>
  auto try_parse_class_bases(Iter begin, Iter end) {
    return try_parse_T(begin, end, rules::class_inheritances);
  }

  template <class Iter>
  auto try_parse_scope_begin(Iter begin, Iter end) {
    return try_parse_T(begin, end, rules::scope_begin);
  }

  template <class Iter>
  ResultT<Iter, std::optional<rules::ast::Class>> try_parse_entire_class(
      Iter begin, Iter end) {
    StdParserState parser;

    struct {
      Iter begin_;
      Iter end_;

      // NOTE: no source location for generated code
      std::uint16_t row = 0;
      std::uint16_t col = 0;

      auto get_row() { return row; }
      auto get_column() { return col; }

      Iter begin() { return begin_; }
      Iter end() { return end_; }

      explicit operator bool() const { return begin_ != end_; }
    } source{begin, end};

    while (source) {
      if (auto out = parser.parse(source)) {
        source.begin_ = out->processed_to;
      } else {
        return {source.begin_, std::nullopt};
      }
    }

    auto& code_fragment = parser.get_top_code_fragment();
    auto& code_fragments = code_fragment.get_all_code_fragments();
    if (code_fragments.size() != 1) {
      return {begin, std::nullopt};
    }

    auto& first_code_fragment = code_fragments.front();
    if (std::holds_alternative<rules::ast::Class>(first_code_fragment)) {
      return {begin, {std::get<rules::ast::Class>(first_code_fragment)}};
    }

    return {begin, std::nullopt};
  }

  template <class Source>
  auto parse(Source& source) {
    return parser.parse(source);
  }

  template <class Source>
  auto parse_include(Source& source) {
    auto begin = source.begin();
    auto end = source.end();

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(begin, end,
                            // rules begin
                            rules::include
                            // rules end
    );

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  // ========
  // INCLUDES
  // ========

  template <class Source>
  auto get_includes(Source& source) {
    return parser.get_includes(source);
  }

  /**
   * Return all the includes in the parsed file
   */
  auto& get_all_includes() { return parser.get_all_includes(); }

  // ==============
  // CODE FRAGMENTS
  // ==============

  /**
   * Add a new code_fragment
   */
  void open_new_code_fragment(CodeFragment&& arg) {
    parser.open_new_code_fragment(std::move(arg));
  }

  /**
   * Return a constant view of all the code_fragments
   */
  auto const& get_all_code_fragments() const {
    return parser.get_all_code_fragments();
  }

  CodeFragment& get_current_code_fragment() {
    return parser.get_current_code_fragment();
  }

  // TODO: constrain Types as the types in the variant of CodeFragment
  template <class... Types>
  bool is_current_code_fragment() {
    auto& current = parser.get_current_code_fragment();
    return (std::holds_alternative<Types>(current) || ...);
  }

  // TODO: Constraint Fragment to CodeFragment
  template <class Fragment>
  void close_code_fragment() {
    parser.close_code_fragment<Fragment>();
  }
};

}  // namespace std_parser

#endif  //! STD_PARSER_H
