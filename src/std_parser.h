#ifndef STD_PARSER_H
#define STD_PARSER_H

#include <variant>

#include <std_rules.h>

namespace std_parser {
class StdParser {
  // TODO: think up a name for this
  using something = std::variant<rules::ast::Namespace, rules::ast::Class,
                                 rules::ast::Function>;
  rules::ast::Namespace global_namespace{""};
  std::vector<something> nestings;

  // parser methods

  template <class Source>
  void parse_inside_namespace(Source& source) {
    auto begin = source.begin();
    auto end = source.end();

    auto cs = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      nestings.push_back(rez.name);
    };
    auto fun = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      nestings.push_back(rez.name);
    };
    auto var = [this](auto& ctx) {
      auto& rez = _attr(ctx);
      // NOTE: clang++ 5/6 bug use g++ 7 for now
      // TODO: can we change to visit?
      auto& current = std::get<rules::ast::Namespace>(nestings.back());
      current.add_variable(rez);
    };

    auto sb = [this](auto& ctx) {
      nestings.emplace_back(rules::ast::Namespace{""});
    };

    auto se = [this](auto& ctx) {
      auto&& current =
          std::move(std::get<rules::ast::Namespace>(nestings.back()));
      nestings.pop_back();
      if (nestings.empty()) {
        global_namespace.add_namespace(std::move(current));
      } else {
        // TODO: move the current one to the previous one
      }
    };

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(
        begin, end,
        // rules begin
        rules::optionaly_space >>
            ((rules::class_or_struct >> rules::scope_begin)[cs] |
             (rules::class_or_struct >> rules::statement_end) |
             (rules::function_signiture >> rules::scope_begin)[fun] |
             (rules::function_signiture >> rules::statement_end) |
             rules::scope_begin[sb] | rules::scope_end[se] | rules::var[var])
        // rules end
    );

    if (parsed) {
    }
  }

 public:
  template <class Source>
  auto parse(Source& source) {
    // TODO: see the type of the last nesting
    return parse_inside_namespace(source);
  }
};
}  // namespace std_parser

#endif  //! STD_PARSER_H
