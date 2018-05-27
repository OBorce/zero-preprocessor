#ifndef STATIC_REFLECTION_H
#define STATIC_REFLECTION_H

#include <variant>

#include <boost/spirit/home/x3.hpp>

#include <result.hpp>
#include <std_ast.hpp>

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
    out += "\nfriend reflect::Reflect<";
    out += c.name;
    out += ">;\n};\n";

    auto& templates = c.template_parameters.template_parameters;
    std::string type_out;
    if (!templates.empty()) {
      type_out += '<';
      for (auto& type : templates) {
        //TODO: this will not work for non type templates
        type_out += "class ";
        type_out += type;
        type_out += ',';
      }
      type_out.pop_back();
      type_out += '>';
    }

    if (c.is_templated()) {
      out += "template ";
      out += type_out;
      out += "struct reflect::Reflect<";
    } else {
      out += "template <> struct reflect::Reflect<";
    }
    out += c.name;
    std::string template_types;
    if (c.is_templated()) {
      template_types += '<';
      for (auto& type : templates) {
        template_types += type;
        template_types += ',';
      }
      template_types.pop_back();
      template_types += '>';
      out += template_types;
    }
    out += "> {\n";

    out += "constexpr inline static std::tuple public_data_members = {";
    for (auto& kv : c.public_members) {
      out += '&';
      out += c.name;
      if (c.is_templated()) {
        out += template_types;
      }
      out += "::";
      out += kv.name;
      out += ',';
    }

    if (!c.public_members.empty()) {
      out.pop_back();
    }
    out += "};\n";

    out += "using public_data_member_types = std::tuple<";
    for (auto& m : c.public_members) {
      out += m.type.to_string();
      out += ',';
    }

    if (!c.public_members.empty()) {
      out.pop_back();
    }
    out += ">;\n";

    out += "constexpr static auto name = \"";
    out += c.name;
    out += "\";\n";

    out += "static constexpr auto object_type = \"";
    switch (c.type) {
      case std_parser::rules::ast::class_type::CLASS:
        out += "reflect::ObjectType::CLASS;";
        break;
      case std_parser::rules::ast::class_type::STRUCT:
        out += "reflect::ObjectType::STRUCT;";
        break;
    }
    out += "\";\n";

    out += "};";

    return out;
  }

  template <class V>
  bool is_class(V v) {
    return std::holds_alternative<Class>(v);
  }

  auto generate_reflection() {
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();
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
  constexpr static int id = 5;

  StaticReflexParser(Parent& p) : parent{p} {}

  template <class Source>
  auto parse(Source& source) {
    auto& std_parser = parent.template get_parser<Parent::std_parser_id>();

    // TODO: parse $reflexpr
    return is_class(std_parser.get_current_nesting())
               ? parse_end_of_class(source)
               : std::nullopt;
  }
};
}  // namespace static_reflection

#endif  //! STATIC_REFLECTION_H
