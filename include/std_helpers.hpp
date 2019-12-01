#ifndef STD_HELPERS_HPP
#define STD_HELPERS_HPP

#include <ostream>
#include <string>

#include <overloaded.hpp>
#include <std_ast.hpp>

namespace std_parser::rules::ast {

template <class Collection, class Separator>
std::string join(Collection const& col, Separator const& s) {
  std::string out;
  auto size = std::size(col);
  if (size == 0) {
    return out;
  }

  out.reserve(100);
  for (std::size_t i = 0; i < size - 1; ++i) {
    out += col[i];
    out += s;
  }

  out += col[size - 1];

  return out;
}

template <class Collection, class Lambda, class Separator>
std::string join(Collection const& col, Lambda l, Separator s) {
  std::string out;
  auto size = std::size(col);
  if (size == 0) {
    return out;
  }

  out.reserve(100);
  for (std::size_t i = 0; i < size - 1; ++i) {
    out += l(col[i]);
    out += s;
  }

  out += l(col[size - 1]);

  return out;
}

std::string to_string(Literal const& x) {
  return std::visit(overloaded{
                        [](std::vector<int64_t> const& v) -> std::string {
                          return join(
                              v, [](int64_t i) { return std::to_string(i); },
                              '\'');
                        },
                        [](std::string const& l) { return l; },
                        [](double l) { return std::to_string(l); },
                        [](char l) { return std::to_string(l); },
                    },
                    x.lit);
}

std::string to_string(Type const& x);

std::string to_string(unqulified_type const& x) {
  std::string type_out = join(x.name, "::");

  auto& templates = x.template_types.template_types;
  if (!templates.empty()) {
    type_out += '<';
    for (auto& tmpl : templates) {
      type_out += std::visit(overloaded{
          [&](LiteralExpression const& x) {return to_string(x.lit);},
          [&](auto& x) {return to_string(x);},
          }, tmpl);
      type_out += ',';
    }
    type_out.back() = '>';
  }

  return type_out;
}

std::string to_string(UnqulifiedType const& x) {
  return join(
      x.type, [](unqulified_type const& ut) { return to_string(ut); }, "::");
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
