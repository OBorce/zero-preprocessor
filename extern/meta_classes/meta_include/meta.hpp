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

enum class Virtual { YES, NO };

enum class Constructor { CONSTRUCTOR, DESTRUCTOR, NOTHING };

struct Function {
  CppType return_type;
  Virtual virtual_status;
  Constructor constructor_type;
  std::string name;
  std::vector<Param> parameters;
  Access access = Access::UNSPECIFIED;

  std::string body = "";

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

  bool has_access() { return access != Access::UNSPECIFIED; }

  void make_public() { access = Access::PUBLIC; }

  void make_private() { access = Access::PRIVATE; }

  void make_protected() { access = Access::PROTECTED; }

  bool is_public() { return access == Access::PUBLIC; }

  bool is_protected() { return access == Access::PROTECTED; }

  bool is_private() { return access == Access::PRIVATE; }

  void make_pure_virtual() {
    virtual_status = Virtual::YES;
    body = " = 0;\n";
  }

  std::string get() {
    std::string s;
    s.reserve(100);
    if (virtual_status == Virtual::YES) {
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
    s += body;
    if (body.empty()) {
      s += ';';
      s += '\n';
    }

    return s;
  }
};

struct Type {
  std::vector<Function> methods;
  std::vector<Var> variables;
  std::string body;

  Type(std::vector<Function>&& m, std::vector<Var>&& v)
      : methods{std::move(m)}, variables{std::move(v)}, body{} {}
};

class type {
  const std::string class_name;
  std::shared_ptr<Type> internal;

 public:
  type(std::string&& name, std::vector<Function>&& methods,
       std::vector<Var>&& variables)
      : class_name{std::move(name)},
        internal{
            std::make_shared<Type>(std::move(methods), std::move(variables))} {}

  auto const& name() const { return class_name; }

  auto const& functions() const { return internal->methods; }

  auto const& variables() const { return internal->variables; }

  auto& operator<<(std::string_view s) {
    internal->body += s;
    return *this;
  }

  auto& operator<<(Function& f) {
    internal->body += f.get();
    return *this;
  }

  std::string get_representation() {
    std::string content;
    // TODO: calculate size of string upfront
    content.reserve(100);
    content += "struct ";
    content += class_name;
    content += " {\n";
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
  int a;
  std::cin >> a;
  Virtual virtual_status = static_cast<Virtual>(a);
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

  return {std::move(return_type), virtual_status,    constructor_type,
          std::move(name),        std::move(params), acc};
}

type read_type() {
  std::string class_name;
  std::cin >> class_name;
  std::size_t num_methods, num_variables;

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

  return {std::move(class_name), std::move(methods), std::move(variables)};
}
}  // namespace meta

#endif  // META_H
