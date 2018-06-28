#ifndef STD_PARSER_H
#define STD_PARSER_H

#include <any>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <variant>

#include <overloaded.hpp>
#include <result.hpp>
#include <std_rules.hpp>
#include <string_utils.hpp>

namespace std_parser {
using CodeFragment =
    std::variant<rules::ast::Namespace, rules::ast::Scope, rules::ast::Class,
                 rules::ast::Enumeration, rules::ast::Function,
                 rules::ast::Expression, rules::ast::RoundExpression,
                 rules::ast::CurlyExpression>;

class StdParserState {
  std::vector<CodeFragment> code_fragments = {rules::ast::Namespace{""}};
  std::unordered_set<std::string> includes;

  // some iterator pointing to the start of the function's body
  // FIXME: used to capture a function's body used by the metaclasses
  // until we can recreate the body from our AST
  std::vector<std::any> function_begins;

  // parser methods

  template <class Source>
  auto parse_inside_namespace(Source& source, rules::ast::Namespace& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      code_fragments.emplace_back(std::move(rez));
    };

    auto fun = [this, &begin](auto& ctx) {
      auto& rez = _attr(ctx);
      // TODO: we can get begin from the context
      function_begins.push_back(begin);
      code_fragments.emplace_back(std::move(rez));
    };

    auto var = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.add_variable(std::move(rez));
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto sb = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      code_fragments.emplace_back(rules::ast::Namespace{std::move(rez)});
    };

    auto se = [&](auto&) {
      if (code_fragments.size() < 2) {
        // NOTE: more closing brackets than opened
        throw std::runtime_error("extraneous closing brace ('}')");
      }

      auto& v = code_fragments[code_fragments.size() - 2];
      std::visit(
          overloaded{
              [&](rules::ast::Namespace& arg) {
                arg.add_namespace(std::move(current));
              },
              [](auto) {
                /* NOTE: Can't have a namespace inside a class, function or a
                 * local scope*/
              },
          },
          v);
      code_fragments.pop_back();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::some_space |
                      (rules::optionaly_space >>
                       ((rules::class_or_struct >> rules::scope_begin)[nest] |
                        (rules::class_or_struct >> rules::statement_end) |
                        (rules::function_signiture >> rules::scope_begin)[fun] |
                        (rules::function_signiture >> rules::statement_end) |
                        (rules::operator_signiture >> rules::scope_begin)[fun] |
                        (rules::operator_signiture >> rules::statement_end) |
                        (rules::enumeration >> rules::scope_begin)[nest] |
                        (rules::enumeration >> rules::statement_end) |
                        rules::namespace_begin[sb] | rules::scope_end[se] |
                        rules::include[inc] | rules::comment | rules::var[var]))
                  // rules end
        );

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  template <class Source>
  auto parse_inside_class(Source& source, rules::ast::Class& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      code_fragments.emplace_back(std::move(rez));
    };

    auto fun = [this, &begin](auto& ctx) {
      auto& rez = _attr(ctx);
      // TODO: we can get begin from the context
      function_begins.push_back(begin);
      code_fragments.emplace_back(std::move(rez));
    };

    auto funSig = [&](auto& ctx) {
      auto& rez = _attr(ctx);
      current.add_function(std::move(rez));
    };

    auto ac = [&](auto& ctx) {
      auto& rez = _attr(ctx);
      current.set_access_modifier(rez);
    };

    auto var = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.add_variable(std::move(rez));
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
        rules::optionaly_space >>
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
             rules::class_access_modifier[ac] | rules::var[var])
        // rules end
    );

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  template <class Source>
  auto parse_inside_enum(Source& source, rules::ast::Enumeration& current) {
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

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  template <class Source>
  auto parse_inside_expression(Source& source,
                               rules::ast::Expression& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto se = [this](auto&) { close_code_fragment<rules::ast::Expression>(); };

    auto round = [this](auto&) {
      code_fragments.emplace_back(rules::ast::RoundExpression{});
    };

    auto curly = [this](auto&) {
      code_fragments.emplace_back(rules::ast::CurlyExpression{});
    };

    auto e = [&](auto&) { current.is_begin = false; };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;
    // TODO: when is this even true?
    if (current.is_begin) {
      parsed = x3::parse(begin, end,
                         // rules begin
                         rules::optionaly_space >>
                             (rules::comment | rules::parenthesis_begin[round] |
                              rules::statement_end[se] | rules::expression2[e])
                         // rules end
      );
    } else {
      parsed = x3::parse(
          begin, end,
          // rules begin
          rules::optionaly_space >>
              (rules::statement_end[se] | rules::comment |
               rules::parenthesis_begin[round] | rules::curly_begin[curly] |
               (rules::operator_sep >>
                (rules::parenthesis_begin[round] | rules::expression2)[e]))
          // rules end
      );
    }

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  template <class Source>
  auto parse_inside_round_expression(Source& source,
                                     rules::ast::RoundExpression& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto se = [this](auto&) { close_code_fragment<rules::ast::Expression>(); };

    auto round = [this](auto&) {
      code_fragments.emplace_back(rules::ast::RoundExpression{});
    };

    auto curly = [this](auto&) {
      code_fragments.emplace_back(rules::ast::CurlyExpression{});
    };

    auto e = [&](auto&) { current.is_begin = false; };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;
    if (current.is_begin) {
      parsed =
          x3::parse(begin, end,
                    // rules begin
                    rules::optionaly_space >>
                        (rules::comment |
                         // paren expression
                         rules::parenthesis_begin[round] |
                         rules::parenthesis_end[se] | rules::expression2[e])
                    // rules end
          );
    } else {
      parsed = x3::parse(
          begin, end,
          // rules begin
          rules::optionaly_space >>
              (rules::comment |
               // paren expression
               rules::parenthesis_begin[round] | rules::parenthesis_end[se] |
               rules::curly_begin[curly] |
               (rules::operator_sep >>
                (rules::parenthesis_begin[round] | rules::expression2)[e]))
          // rules end
      );
    }

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  template <class Source>
  auto parse_inside_curly_expression(Source& source,
                                     rules::ast::CurlyExpression& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto se = [this](auto&) { close_code_fragment<rules::ast::Expression>(); };

    auto round = [this](auto&) {
      code_fragments.emplace_back(rules::ast::RoundExpression{});
    };

    auto curly = [this](auto&) {
      code_fragments.emplace_back(rules::ast::CurlyExpression{});
    };

    auto e = [&](auto&) { current.is_begin = false; };

    namespace x3 = boost::spirit::x3;
    bool parsed = false;
    if (current.is_begin) {
      parsed = x3::parse(
          begin, end,
          // rules begin
          rules::optionaly_space >>
              (rules::comment | rules::curly_end[se] | rules::expression2[e])
          // rules end
      );
    } else {
      parsed = x3::parse(
          begin, end,
          // rules begin
          rules::optionaly_space >>
              (rules::comment |
               // paren expression
               rules::parenthesis_begin[round] | rules::curly_begin[curly] |
               rules::curly_end[se] |
               (rules::operator_sep >>
                (rules::parenthesis_begin[round] | rules::expression2)[e]))
          // rules end
      );
    }

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  template <class Source>
  auto parse_inside_function(Source& source, rules::ast::Function& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      code_fragments.emplace_back(std::move(rez));
    };

    auto exp = [this](auto&) {
      code_fragments.emplace_back(rules::ast::Expression{});
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto sb = [&](auto&) { code_fragments.emplace_back(rules::ast::Scope{}); };

    auto se = [&](auto&) {
      // TODO: remove function_begins when not needed
      if (code_fragments.size() < 2 || function_begins.empty()) {
        // NOTE: more closing brackets than opened
        throw std::runtime_error("extraneous closing brace ('}')");
      }
      auto current_functions_start = function_begins.back();
      auto end = source.begin();
      auto start = std::any_cast<decltype(end)>(current_functions_start);
      current.body = std::string(start, end) + '\n';
      auto& v = code_fragments[code_fragments.size() - 2];
      std::visit(
          overloaded{
              [&](rules::ast::Namespace& arg) {
                arg.add_function(std::move(current));
              },
              [&](rules::ast::Class& arg) {
                arg.add_function(std::move(current));
              },
              [](auto) {
                /* NOTE: Can't have a function inside a function or local
                 * scope*/
              },
          },
          v);
      code_fragments.pop_back();
      function_begins.pop_back();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::optionaly_space >>
                      ((rules::class_or_struct >> rules::scope_begin)[nest] |
                       (rules::class_or_struct >> rules::statement_end) |
                       (rules::enumeration >> rules::scope_begin)[nest] |
                       (rules::enumeration >> rules::statement_end) |
                       rules::scope_begin[sb] | rules::scope_end[se] |
                       rules::include[inc] | rules::comment | rules::var |
                       rules::for_loop | rules::if_expression |
                       rules::return_statement | rules::expression2[exp])
                  // rules end
        );

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  template <class Source>
  auto parse_inside_scope(Source& source, rules::ast::Scope&) {
    auto begin = source.begin();
    auto end = source.end();

    auto nest = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      code_fragments.emplace_back(std::move(rez));
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto sb = [&](auto&) { code_fragments.emplace_back(rules::ast::Scope{}); };

    auto se = [&](auto&) {
      if (code_fragments.size() < 2) {
        // NOTE: more closing brackets than opened
        throw std::runtime_error("extraneous closing brace ('}')");
      }
      auto& v = code_fragments[code_fragments.size() - 2];
      std::visit(
          overloaded{
              [&](rules::ast::Function&) {
                // TODO: keep local scope inside function?
              },
              [](auto) {
                /* NOTE: Can't have a function inside a function or local
                 * scope*/
              },
          },
          v);
      code_fragments.pop_back();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(
        begin, end,
        // rules begin
        rules::optionaly_space >>
            ((rules::class_or_struct >> rules::scope_begin)[nest] |
             (rules::class_or_struct >> rules::statement_end) |
             (rules::enumeration >> rules::scope_begin)[nest] |
             (rules::enumeration >> rules::statement_end) |
             rules::scope_begin[sb] | rules::scope_end[se] | rules::statement |
             rules::include[inc] | rules::comment | rules::var |
             rules::for_loop | rules::if_expression | rules::return_statement)
        // rules end
    );

    return parsed ? std::optional{Result{
                        begin, make_string_view(source.begin(), begin)}}
                  : std::nullopt;
  }

  void close_current_class() {
    auto& c = std::get<rules::ast::Class>(code_fragments.back());
    auto& v = code_fragments[code_fragments.size() - 2];
    std::visit(
        overloaded{
            [&](rules::ast::Namespace& arg) { arg.add_class(std::move(c)); },
            [&](rules::ast::Class& arg) { arg.add_class(std::move(c)); },
            [](auto) {
              /* TODO: local classes inside functions or local scopes*/
            },
        },
        v);
  }

  void close_current_enum() {
    auto& c = std::get<rules::ast::Enumeration>(code_fragments.back());
    auto& v = code_fragments[code_fragments.size() - 2];
    std::visit(
        overloaded{
            [&](rules::ast::Namespace& arg) { arg.add_enum(std::move(c)); },
            [&](rules::ast::Class& arg) { arg.add_enum(std::move(c)); },
            [](auto) {
              /* TODO: local enums inside functions or local scopes*/
            },
        },
        v);
  }

 public:
  /**
   * Parse a single statement from the source
   *
   * Returns an `optional<iterator>` if successfully parsed a statement pointing
   * to the end of the parsed statement in the source, else empty
   */
  template <class Source>
  auto parse(Source& source) {
    auto& current_code_fragment = code_fragments.back();
    return std::visit(
        overloaded{
            [&](rules::ast::Namespace& arg) {
              return parse_inside_namespace(source, arg);
            },
            [&](rules::ast::Class& arg) {
              return parse_inside_class(source, arg);
            },
            [&](rules::ast::Function& arg) {
              return parse_inside_function(source, arg);
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
        },
        current_code_fragment);
  }

  /**
   * Add a new code_fragment
   */
  void open_new_code_fragment(CodeFragment&& arg) {
    code_fragments.emplace_back(std::move(arg));
  }

  /**
   * Return a constant view of all the code_fragments
   */
  auto const& get_all_code_fragments() { return code_fragments; }

  CodeFragment& get_current_code_fragment() { return code_fragments.back(); }

  rules::ast::Namespace const& get_top_code_fragment() {
    return std::get<rules::ast::Namespace>(code_fragments.front());
  }

  // TODO add SFINAE for CodeFragment
  template <class Fragment>
  void close_code_fragment() {
    if (code_fragments.size() < 2) {
      // NOTE: more closing brackets than opened
      throw std::runtime_error("extraneous closing brace ('}')");
    }

    if constexpr (std::is_same<Fragment, rules::ast::Class>()) {
      close_current_class();
    } else if constexpr (std::is_same<Fragment, rules::ast::Enumeration>()) {
      close_current_enum();
    } else if constexpr (std::is_same<Fragment, rules::ast::Expression>()) {
      // NOTE: nothing to do for now
      // update when we decide to store the expressions
    } else {
      // TODO: implement
    }

    code_fragments.pop_back();
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
        std::size_t size = std::min(30ul, remaining);
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
  template <class Iter, class Id, class T, template <class, class> class Rule,
            class Out = T>
  auto try_parse_T(Iter begin, Iter end, Rule<Id, T> rule) {
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
  template <class Out, class Iter, class Id, class T,
            template <class, class> class Rule>
  auto try_parse_T(Iter begin, Iter end, Rule<Id, T> rule) {
    return try_parse_T<Iter, Id, T, Rule, Out>(begin, end, rule);
  }

  StdParserState parser;

 public:
  // TODO: when supported in std=c++2a change to fixed length string
  constexpr static int id = 's' + 't' + 'd';

  template <class Iter>
  auto try_parse_function(Iter begin, Iter end) {
    return try_parse_T<rules::ast::Function>(
        begin, end, rules::function_signiture >> rules::scope_begin);
  }

  template <class Iter>
  auto try_parse_variable(Iter begin, Iter end) {
    return try_parse_T(begin, end, rules::var);
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
    auto& classes = code_fragment.get_all_classes();
    if (classes.size() != 1) {
      return {begin, std::nullopt};
    }

    return {begin, {classes.begin()->second}};
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
  auto const& get_all_code_fragments() {
    return parser.get_all_code_fragments();
  }

  CodeFragment& get_current_code_fragment() {
    return parser.get_current_code_fragment();
  }

  // TODO: Constraint Fragment to CodeFragment
  template <class Fragment>
  void close_code_fragment() {
    parser.close_code_fragment<Fragment>();
  }
};

}  // namespace std_parser

#endif  //! STD_PARSER_H
