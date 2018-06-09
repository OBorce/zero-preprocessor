#ifndef STD_HELPERS_HPP
#define STD_HELPERS_HPP

#include <string>

#include <std_ast.hpp>

namespace std_parser::rules::ast {

std::string to_string(Type const& x);

std::string to_string(UnqulifiedType const& x) {
  std::string type_out;
  type_out.reserve(30);
  for (auto& t : x.name) {
    type_out += t;
    type_out += "::";
  }
  type_out.pop_back();
  type_out.pop_back();

  auto& templates = x.template_types.template_types;
  if (!templates.empty()) {
    type_out += '<';
    for (auto& type : templates) {
      type_out += to_string(type);
    }
    type_out += '>';
  }

  return type_out;
}

std::string to_string(Type const& x) {
  std::string type_out;
  type_out.reserve(30);

  type_out += to_string(x.type);

  return type_out;
}

}  // namespace std_parser::rules::ast

#endif  // STD_HELPERS_HPP
