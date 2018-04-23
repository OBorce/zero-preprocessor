#ifndef STATIC_REFLECTION_H
#define STATIC_REFLECTION_H

#include <variant>

#include <boost/spirit/home/x3.hpp>

#include <result.h>
#include <std_ast.h>

namespace static_reflection {
namespace rules {
namespace x3 = boost::spirit::x3;

using x3::eol;
using x3::lit;

x3::rule<class optionaly_space> const optionaly_space = "optionaly_space";
auto const optionaly_space_def = *(eol | ' ' | '\t');

x3::rule<class scope_end> const scope_end = "scope_end";
auto const scope_end_def = optionaly_space >> '}' >> optionaly_space >>
                           lit(';');

BOOST_SPIRIT_DEFINE(optionaly_space, scope_end);
}  // namespace rules

template <class Parent>
class StaticReflexParser {
  using Class = std_parser::rules::ast::Class;
  // TODO: generate reflection for the current class
  auto generate_class_reflection(Class& c) {
    std::string out;
    out.reserve(200);
    out += "\n};\n";
    out += "namespace reflect {template <class T> struct Reflect;}\n";
    out += "template <> struct reflect::Reflect<";
    out += c.name;
    out += "> { constexpr inline static std::tuple members = {";
    for (auto& kv : c.public_members) {
      out += '&';
      out += c.name;
      out += "::";
      out += kv.name;
      out += ',';
    }

    out.pop_back();
    out += "};\n constexpr static auto name = \"";
    out += c.name;
    out += "\";\n};";

    return out;
  }

  template <class V>
  bool is_class(V v) {
    return std::holds_alternative<Class>(v);
  }

  auto generate_reflection() {
    auto& std_parser = parent.template get_parser<1>();
    auto& current_class = std::get<Class>(std_parser.get_current_nesting());
    auto rez = generate_class_reflection(current_class);
    std_parser.close_current_class();
    return rez;
  }

  template <typename Source>
  auto parse_end_of_class(Source& source) {
    auto begin = source.begin();
    auto end = source.end();

    namespace x3 = boost::spirit::x3;
    bool parsed = x3::parse(begin, end, rules::scope_end);

    return parsed ? std::optional{Result{begin, generate_reflection()}}
                  : std::nullopt;
  }

  Parent& parent;

 public:
  // TODO: when supported in std=c++2a change to fixed length string

  StaticReflexParser(Parent& p) : parent{p} {}

  constexpr static int id = 5;
  template <class Source>
  auto parse(Source& source) {
    auto& std_parser = parent.template get_parser<1>();

    // TODO: parse $reflexpr
    return is_class(std_parser.get_current_nesting())
               ? parse_end_of_class(source)
               : std::nullopt;
  }
};
}  // namespace static_reflection

#endif  //! STATIC_REFLECTION_H
