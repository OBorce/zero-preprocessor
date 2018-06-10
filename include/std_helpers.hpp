#ifndef STD_HELPERS_HPP
#define STD_HELPERS_HPP

#include <ostream>
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
  if (!x.name.empty()) {
    type_out.pop_back();
    type_out.pop_back();
  }

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

template <class T>
void serialize(T& out, Type const& x) {
  out << x.left_qualifiers.size() << '\n';
  for (auto q : x.left_qualifiers) {
    out << static_cast<int>(q) << '\n';
  }

  auto t = to_string(x.type);
  out << t.empty() << '\n';
  if (!t.empty()) {
    out << t << '\n';
  }

  out << x.right_qualifiers.size() << '\n';
  for (auto q : x.right_qualifiers) {
    out << static_cast<int>(q) << '\n';
  }
}

}  // namespace std_parser::rules::ast

#endif  // STD_HELPERS_HPP
