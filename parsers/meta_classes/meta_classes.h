#ifndef META_CLASSES_H
#define META_CLASSES_H

#include <unordered_set>
#include <variant>

#include <meta_classes_rules.h>
#include <result.h>
#include <std_ast.h>
#include <string_utils.h>

namespace meta_classes {
template <typename Parent>
class MetaClassParser {
  template <class Writer>
  auto parse_meta_calss_function(std::string_view source, Writer& writer) {
    auto& std_parser = parent.template get_parser<1>();
    auto out = std_parser.parse_function(source);

    if (out) {
      auto out = std_parser.parse(source);
      // if inside a constexpr function add it as a meta function
      auto& current_nesting = std_parser.get_current_nesting();
      using Fun = std_parser::rules::ast::Function;
      if (std::holds_alternative<Fun>(current_nesting)) {
        Fun& fun = std::get<Fun>(current_nesting);
        meta_classes.emplace(fun.name);
        inside_meta_class_function = true;

        writer((*out).result);
      }
    }

    return out;
  }

  template <class Source, class Writer>
  auto parse_constexpr_function(Source& source, Writer& writer) {
    auto begin = source.begin();
    auto end = source.end();

    namespace x3 = boost::spirit::x3;
    // TODO: try to parse templates before constexpr
    bool parsed = x3::parse(begin, end, rules::const_expression);

    return parsed
               ? parse_meta_calss_function(make_string_view(begin, end), writer)
               : std::nullopt;
  }

  template <class Source, class Writer>
  auto parse_target(Source& source, Writer& writer) {
    auto begin = source.begin();
    auto end = source.end();

    namespace x3 = boost::spirit::x3;
    // TODO: try to parse templates before constexpr
    bool parsed = x3::parse(begin, end, rules::target);

    return parsed
               ? parse_meta_calss_function(make_string_view(begin, end), writer)
               : std::nullopt;
  }

  template <class Source, class Writer>
  auto parse_inside_constexpr_function(Source& source, Writer& writer) {
    auto& std_parser = parent.template get_parser<1>();
    auto out = std_parser.parse(source);
    if (out) {
      writer((*out).result);

      // if end of function inside_meta_class_function = false;
      auto& current_nesting = std_parser.get_current_nesting();
      using Fun = std_parser::rules::ast::Function;
      if (!std::holds_alternative<Fun>(current_nesting)) {
        inside_meta_class_function = false;
      }
    }

    // TODO: try to parse -> (target)
    return out;
  }

  // TODO: when supported in std=c++2a change to fixed length string
  constexpr static int id = 7;

  Parent& parent;
  std::string meta_exe;
  std::unordered_set<std::string> meta_classes;
  bool inside_meta_class_function = false;

 public:
  MetaClassParser(Parent& p, std::string_view meta_exe)
      : parent{p}, meta_exe{meta_exe} {
    // TODO: here or onInit call the the meta_exe to fill the meta_classes
  }

  /**
   * Parse constexpr meta class function
   * and write them using the writer
   */
  template <class Source, class Writer>
  auto parse_meta_classes(Source& source, Writer& writer) {
    return inside_meta_class_function
               ? parse_inside_constexpr_function(source, writer)
               : parse_constexpr_function(source, writer);
  }

  /**
   * Parse a Meta class
   */
  template <class Source>
  auto parse(Source& source) {
    // TODO: try to parse a meta class
  }
};
}  // namespace meta_classes

#endif  //! META_CLASSES_H
