#ifndef META_H
#define META_H

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

static struct Compiler {
  void require(bool b, std::string_view msg) {
    if (!b) {
      std::cout << -1 << std::endl;
      std::cout << msg.size() << std::endl;
      std::cout << msg << std::endl;
      std::terminate();
    }
  }

  void error(std::string_view msg) {
    std::cout << -1 << std::endl;
    std::cout << msg.size() << std::endl;
    std::cout << msg << std::endl;
    std::terminate();
  }
} compiler;

namespace meta {

enum class Access { PUBLIC, PROTECTED, PRIVATE, UNSPECIFIED };

enum class TypeQualifier { CONST, CONSTEXPR, L_REF, R_REF, POINTER };

std::string to_string(TypeQualifier q) {
  switch (q) {
    case TypeQualifier::CONST:
      return "const";
    case TypeQualifier::CONSTEXPR:
      return "constexpr";
    case TypeQualifier::L_REF:
      return "&";
    case TypeQualifier::R_REF:
      return "&&";
    case TypeQualifier::POINTER:
      return "*";
  }

  return "";
}

struct CppType {
  std::vector<TypeQualifier> left_qualifiers;
  std::string type;
  std::vector<TypeQualifier> right_qualifiers;

  std::string to_string() {
    std::string out;
    out.reserve(50);
    for (auto q : left_qualifiers) {
      out += meta::to_string(q);
      out += ' ';
    }
    out += type;
    for (auto q : right_qualifiers) {
      out += ' ';
      out += meta::to_string(q);
    }
    return out;
  }
};

struct Param {
  CppType type;
  std::string name;
};

struct Var {
  CppType type;
  std::string name;
  Access access = Access::UNSPECIFIED;

  bool has_access() { return access != Access::UNSPECIFIED; }

  bool is_public() { return access == Access::PUBLIC; }

  bool is_protected() { return access == Access::PROTECTED; }

  bool is_private() { return access == Access::PRIVATE; }

  void make_public() { access = Access::PUBLIC; }

  void make_private() { access = Access::PRIVATE; }

  void make_protected() { access = Access::PROTECTED; }
};

enum class Constructor { CONSTRUCTOR, DESTRUCTOR, NOTHING };
enum class MethodQualifier { NONE, L_REF, R_REF };

struct Function {
  CppType return_type;
  bool is_virtual_;
  Constructor constructor_type;
  std::string name;
  std::vector<Param> parameters;
  bool is_const;
  MethodQualifier qualifier;
  bool is_override;
  Access access = Access::UNSPECIFIED;

  std::string body = "";

  bool is_destructor() { return constructor_type == Constructor::DESTRUCTOR; }

  bool is_copy() {
    if (constructor_type != Constructor::CONSTRUCTOR) {
      return false;
    }

    if (parameters.size() != 1) {
      return false;
    }

    auto& param = parameters.back().type;
    auto qualifiers = param.left_qualifiers;
    qualifiers.insert(qualifiers.end(), param.right_qualifiers.begin(),
                      param.right_qualifiers.end());
    if (qualifiers.size() != 2) {
      return false;
    }

    if (qualifiers.front() != TypeQualifier::CONST) {
      return false;
    }

    if (qualifiers.back() != TypeQualifier::L_REF) {
      return false;
    }

    return name == param.type;
  }

  bool is_move() {
    if (constructor_type != Constructor::CONSTRUCTOR) {
      return false;
    }

    if (parameters.size() != 1) {
      return false;
    }

    auto& param = parameters.back().type;
    if (!param.left_qualifiers.empty()) {
      return false;
    }

    if (param.right_qualifiers.size() != 1) {
      return false;
    }

    if (param.right_qualifiers.front() != TypeQualifier::R_REF) {
      return false;
    }

    return name == param.type;
  }

  bool is_virtual() { return is_virtual_; }

  bool has_access() { return access != Access::UNSPECIFIED; }

  void make_public() { access = Access::PUBLIC; }

  void make_private() { access = Access::PRIVATE; }

  void make_protected() { access = Access::PROTECTED; }

  bool is_public() { return access == Access::PUBLIC; }

  bool is_protected() { return access == Access::PROTECTED; }

  bool is_private() { return access == Access::PRIVATE; }

  void make_pure_virtual() {
    is_virtual_ = true;
    body = " = 0;\n";
  }

  std::string to_string() {
    std::string s;
    s.reserve(100);
    if (is_virtual_) {
      s += "virtual ";
    }
    s += return_type.to_string();
    s += ' ';
    s += name;
    s += '(';
    for (auto& p : parameters) {
      s += p.type.to_string();
      s += ' ';
      s += p.name;
      s += ',';
    }
    if (!parameters.empty()) {
      s.pop_back();
    }
    s += ")";
    if (is_const) {
      s += " const";
    }
    switch (qualifier) {
      case MethodQualifier::L_REF:
        s += " &";
        break;
      case MethodQualifier::R_REF:
        s += " &&";
        break;
      default:
        break;
    }

    if (is_override) {
      s += " override";
    }
    s += body;
    if (body.empty()) {
      s += ';';
      s += '\n';
    }

    return s;
  }
};

struct Base {
  std::string name;
  Access access = Access::UNSPECIFIED;

  bool has_access() { return access != Access::UNSPECIFIED; }

  void make_public() { access = Access::PUBLIC; }

  std::string to_string() {
    std::string out;
    out.reserve(20);
    switch (access) {
      case Access::PUBLIC:
        out += "public ";
        break;
      case Access::PROTECTED:
        out += "protected ";
        break;
      case Access::PRIVATE:
        out += "private ";
        break;
      case Access::UNSPECIFIED:
        break;
    }
    out += name;

    return out;
  }
};

struct Type {
  std::vector<Function> methods;
  std::vector<Base> bases;
  std::vector<Var> variables;
  std::string body;

  Type() = default;

  Type(std::vector<Function>&& m, std::vector<Var>&& v,
       std::vector<Base>&& bases)
      : methods{std::move(m)},
        bases{std::move(bases)},
        variables{std::move(v)},
        body{} {}
};

class type {
  const std::string class_name;
  std::shared_ptr<Type> internal;

 public:
  type(std::string&& name, std::vector<Function>&& methods,
       std::vector<Var>&& variables, std::vector<Base>&& bases)
      : class_name{std::move(name)},
        internal{std::make_shared<Type>(
            std::move(methods), std::move(variables), std::move(bases))} {}

  type(std::string name)
      : class_name{std::move(name)}, internal{std::make_shared<Type>()} {}

  auto const& name() const { return class_name; }

  auto const& functions() const { return internal->methods; }

  auto const& bases() const { return internal->bases; }

  auto const& variables() const { return internal->variables; }

  auto& operator<<(std::string_view s) {
    internal->body += s;
    return *this;
  }

  auto& operator<<(Function& f) {
    internal->methods.push_back(f);
    return *this;
  }

  auto& operator<<(Base& b) {
    internal->bases.push_back(b);
    return *this;
  }

  std::string get_representation() {
    std::string content;
    content.reserve(100);
    content += "struct ";
    content += class_name;
    if (!internal->bases.empty()) {
      content += ':';
      for (auto& b : internal->bases) {
        content += b.to_string();
        content += ',';
      }
      content.pop_back();
    }
    content += " {\n";
    if (!internal->methods.empty()) {
      for (auto& f : internal->methods) {
        content += f.to_string();
        content += '\n';
      }
      content.pop_back();
    }
    content += internal->body;
    content += "\n};";

    return content;
  }
};

CppType read_cpp_type() {
  std::size_t n;
  std::cin >> n;
  std::vector<TypeQualifier> left_qualifiers;
  while (n-- > 0) {
    int q;
    std::cin >> q;
    left_qualifiers.push_back(static_cast<TypeQualifier>(q));
  }

  bool empty;
  std::cin >> empty;
  std::string type;
  if (!empty) {
    std::cin >> type;
  }

  std::cin >> n;
  std::vector<TypeQualifier> right_qualifiers;
  while (n-- > 0) {
    int q;
    std::cin >> q;
    right_qualifiers.push_back(static_cast<TypeQualifier>(q));
  }

  return {std::move(left_qualifiers), std::move(type),
          std::move(right_qualifiers)};
}

Param read_parameter() {
  std::string name;
  auto type = read_cpp_type();
  std::cin >> name;

  return {std::move(type), std::move(name)};
}

Var read_var() {
  std::string name;
  auto type = read_cpp_type();
  int a;
  std::cin >> a;
  Access acc = static_cast<Access>(a);
  std::cin >> name;

  return {std::move(type), std::move(name), acc};
}

Function read_function() {
  auto return_type = read_cpp_type();
  bool is_virtual;
  std::cin >> is_virtual;
  int a;
  std::cin >> a;
  Constructor constructor_type = static_cast<Constructor>(a);
  std::cin >> a;
  Access acc = static_cast<Access>(a);

  std::string name;
  std::cin >> name;
  std::size_t num_params;
  std::cin >> num_params;
  std::vector<Param> params;
  params.reserve(num_params);
  while (num_params-- > 0) {
    params.emplace_back(read_parameter());
  }
  bool is_const;
  std::cin >> is_const;

  std::cin >> a;
  MethodQualifier qualifier = static_cast<MethodQualifier>(a);

  bool is_override;
  std::cin >> is_override;

  return {std::move(return_type),
          is_virtual,
          constructor_type,
          std::move(name),
          std::move(params),
          is_const,
          qualifier,
          is_override,
          acc};
}

Base read_base() {
  std::string name;
  std::cin >> name;
  int a;
  std::cin >> a;
  Access acc = static_cast<Access>(a);

  return {name, acc};
}

type read_type() {
  std::string class_name;
  std::cin >> class_name;
  std::size_t num_methods, num_variables, num_bases;

  std::cin >> num_methods;
  std::vector<Function> methods;
  methods.reserve(num_methods);
  while (num_methods-- > 0) {
    methods.emplace_back(read_function());
  }

  std::cin >> num_variables;
  std::vector<Var> variables;
  variables.reserve(num_variables);
  while (num_variables-- > 0) {
    variables.emplace_back(read_var());
  }

  std::cin >> num_bases;
  std::vector<Base> bases;
  bases.reserve(num_bases);
  while (num_bases-- > 0) {
    bases.emplace_back(read_base());
  }

  return {std::move(class_name), std::move(methods), std::move(variables),
          std::move(bases)};
}
}  // namespace meta

#endif  // META_H
