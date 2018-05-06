#ifndef STD_PARSER_H
#define STD_PARSER_H

#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <variant>

#include <overloaded.h>
#include <result.h>
#include <std_rules.h>

namespace std_parser {

// FIXME: this only works for char
// TODO: change this to a span<T> in 2a
template <typename Iter>
std::string_view make_range(Iter begin, Iter end) {
  std::size_t size = std::distance(begin, end);
  return {std::addressof(*begin), size};
}

class StdParser {
  using Scopable = std::variant<rules::ast::Namespace, rules::ast::Scope,
                                rules::ast::Class, rules::ast::Function>;
  std::vector<Scopable> nestings = {rules::ast::Namespace{""}};
  std::unordered_set<std::string> includes;

  // parser methods

  template <class Source>
  auto parse_inside_namespace(Source& source, rules::ast::Namespace& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto cs = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      nestings.emplace_back(std::move(rez));
    };
    auto fun = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      nestings.emplace_back(std::move(rez));
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
      nestings.emplace_back(rules::ast::Namespace{std::move(rez)});
    };

    auto se = [&](auto& ctx) {
      if (nestings.size() < 2) {
        // NOTE: more closing brackets than opened
        throw std::runtime_error("extraneous closing brace ('}')");
      }

      auto& v = nestings[nestings.size() - 2];
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
      nestings.pop_back();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::some_space |
                      (rules::optionaly_space >>
                       ((rules::class_or_struct >> rules::scope_begin)[cs] |
                        (rules::class_or_struct >> rules::statement_end) |
                        (rules::function_signiture >> rules::scope_begin)[fun] |
                        (rules::function_signiture >> rules::statement_end) |
                        (rules::operator_signiture >> rules::scope_begin)[fun] |
                        (rules::operator_signiture >> rules::statement_end) |
                        rules::namespace_begin[sb] | rules::scope_end[se] |
                        rules::include[inc] | rules::comment | rules::var[var]))
                  // rules end
        );

    return parsed
               ? std::optional{Result{begin, make_range(source.begin(), begin)}}
               : std::nullopt;
  }

  template <class Source>
  auto parse_inside_class(Source& source, rules::ast::Class& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto cs = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      nestings.emplace_back(std::move(rez));
    };
    // TODO: what to do with template types
    auto fun = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      nestings.emplace_back(std::move(rez));
    };
    // TODO: what to do with template types
    auto var = [&current](auto& ctx) {
      auto& rez = _attr(ctx);
      current.add_variable(std::move(rez));
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto se = [&](auto& ctx) { this->close_current_class(); };

    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::optionaly_space >>
                      ((rules::class_or_struct >> rules::scope_begin)[cs] |
                       (rules::class_or_struct >> rules::statement_end) |
                       (rules::function_signiture >> rules::scope_begin)[fun] |
                       (rules::function_signiture >> rules::statement_end) |
                       (rules::operator_signiture >> rules::scope_begin)[fun] |
                       (rules::operator_signiture >> rules::statement_end) |
                       rules::scope_end[se] | rules::include[inc] |
                       rules::comment | rules::var[var])
                  // rules end
        );

    return parsed
               ? std::optional{Result{begin, make_range(source.begin(), begin)}}
               : std::nullopt;
  }

  template <class Source>
  auto parse_inside_function(Source& source, rules::ast::Function& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto cs = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      nestings.emplace_back(std::move(rez));
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto sb = [&](auto& ctx) { nestings.emplace_back(rules::ast::Scope{}); };

    auto se = [&](auto& ctx) {
      if (nestings.size() < 2) {
        // NOTE: more closing brackets than opened
        throw std::runtime_error("extraneous closing brace ('}')");
      }
      auto& v = nestings[nestings.size() - 2];
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
      nestings.pop_back();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed =
        x3::parse(begin, end,
                  // rules begin
                  rules::optionaly_space >>
                      ((rules::class_or_struct >> rules::scope_begin)[cs] |
                       (rules::class_or_struct >> rules::statement_end) |
                       rules::scope_begin[sb] | rules::scope_end[se] |
                       rules::statement | rules::include[inc] | rules::comment |
                       rules::var | rules::return_statement)
                  // rules end
        );

    return parsed
               ? std::optional{Result{begin, make_range(source.begin(), begin)}}
               : std::nullopt;
  }

  template <class Source>
  auto parse_inside_scope(Source& source, rules::ast::Scope& current) {
    auto begin = source.begin();
    auto end = source.end();

    auto cs = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      nestings.emplace_back(std::move(rez));
    };

    auto inc = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      includes.emplace(std::move(rez));
    };

    auto sb = [&](auto& ctx) { nestings.emplace_back(rules::ast::Scope{}); };

    auto se = [&](auto& ctx) {
      if (nestings.size() < 2) {
        // NOTE: more closing brackets than opened
        throw std::runtime_error("extraneous closing brace ('}')");
      }
      auto& v = nestings[nestings.size() - 2];
      std::visit(
          overloaded{
              [&](rules::ast::Function& arg) {
                // TODO: keep local scope inside function?
              },
              [](auto) {
                /* NOTE: Can't have a function inside a function or local
                 * scope*/
              },
          },
          v);
      nestings.pop_back();
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(
        begin, end,
        // rules begin
        rules::optionaly_space >>
            ((rules::class_or_struct >> rules::scope_begin)[cs] |
             (rules::class_or_struct >> rules::statement_end) |
             rules::scope_begin[sb] | rules::scope_end[se] | rules::statement |
             rules::include[inc] | rules::comment | rules::var)
        // rules end
    );

    return parsed
               ? std::optional{Result{begin, make_range(source.begin(), begin)}}
               : std::nullopt;
  }

 public:
  // TODO: when supported in std=c++2a change to fixed length string
  constexpr static int id = 1;
  /**
   * Parse a single statement from the source
   *
   * Returns an `optional<iterator>` if successfully parsed a statement pointing
   * to the end of the parsed statement in the source, else empty
   */
  template <class Source>
  auto parse(Source& source) {
    auto& current_nesting = nestings.back();
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
        },
        current_nesting);
  }

  void close_current_class() {
    auto& c = std::get<rules::ast::Class>(nestings.back());
    if (nestings.size() < 2) {
      // NOTE: more closing brackets than opened
      throw std::runtime_error("extraneous closing brace ('}')");
    }
    auto& v = nestings[nestings.size() - 2];
    std::visit(
        overloaded{
            [&](rules::ast::Namespace& arg) { arg.add_class(std::move(c)); },
            [&](rules::ast::Class& arg) { arg.add_class(std::move(c)); },
            [](auto) {
              /* TODO: local classes inside functions or local scopes*/
            },
        },
        v);
    nestings.pop_back();
  }

  Scopable& get_current_nesting() { return nestings.back(); }

  /**
   * Return all the includes in the parsed file
   */
  auto& get_all_includes() { return includes; }

  void reset_includes() { includes.clear(); }
};
}  // namespace std_parser

#endif  //! STD_PARSER_H
