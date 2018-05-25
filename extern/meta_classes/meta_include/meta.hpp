#ifndef META_H
#define META_H

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

static struct Compiler {
  void require(bool b, std::string_view msg) {
    if (!b) {
      std::cerr << msg;
      std::terminate();
    }
  }
} compiler;

namespace meta {

enum class Access { PRIVATE, PROTECTED, PUBLIC };

struct Param {
  std::string type;
  std::string name;
};

struct Var {
  std::string type;
  std::string name;
};

struct Function {
  std::string return_type;
  std::string name;
  std::vector<Param> parameters;
  Access access = Access::PUBLIC;

  bool is_virtual = false;
  std::string body = "";

  bool is_copy() {
    // TODO
    return false;
  }

  bool is_move() {
    // TODO:
    return false;
  }

  bool has_access() {
    // TODO:
    return false;
  }

  void make_public() { access = Access::PUBLIC; }

  bool is_public() { return access == Access::PUBLIC; }

  void make_pure_virtual() {
    is_virtual = true;
    body = " = 0;\n";
  }

  std::string get() {
    std::string s;
    s.reserve(100);
    if (is_virtual) {
      s += "virtual ";
    }
    s += return_type;
    s += ' ';
    s += name;
    s += '(';
    for (auto& p : parameters) {
      s += p.type;
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

Param read_parameter() {
  std::string type;
  std::string name;
  std::cin >> type;
  std::cin >> name;

  return {std::move(type), std::move(name)};
}

Var read_var() {
  std::string type;
  std::string name;
  std::cin >> type;
  std::cin >> name;

  return {std::move(type), std::move(name)};
}

Function read_function() {
  std::string return_type;

  std::cin >> return_type;
  std::string name;
  std::cin >> name;
  int num_params;
  std::cin >> num_params;
  std::vector<Param> params;
  params.reserve(num_params);
  while (num_params-- > 0) {
    params.emplace_back(read_parameter());
  }

  return {std::move(return_type), std::move(name), std::move(params),
          Access::PUBLIC};
}

type read_type() {
  std::string class_name;
  std::cin >> class_name;
  int num_methods, num_variables;

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
