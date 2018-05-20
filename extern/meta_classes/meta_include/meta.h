#ifndef META_H
#define META_H

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace meta {

enum class Access { PRIVATE, PROTECTED, PUBLIC };

struct Param {
  std::string type;
  std::string name;
};

struct Function {
  std::string return_type;
  std::string name;
  std::vector<Param> parameters;
  Access access = Access::PUBLIC;
};

struct Type {
  std::vector<Function> methods;
  std::string body;

  Type(std::vector<Function>&& m): methods{std::move(m)}, body{} {}
};

class type {
  const std::string class_name;
  std::shared_ptr<Type> internal;

 public:
  type(std::string&& name, std::vector<Function>&& methods)
      : class_name{std::move(name)},
        internal{std::make_shared<Type>(std::move(methods))} {}

  auto const& name() const { return class_name; }

  auto& functions() { return internal->methods; }

  auto& operator<<(std::string_view s) {
    internal->body += s;
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
};  // namespace meta

Param read_parameter() {
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
  int num_methods;
  std::cin >> num_methods;
  std::vector<Function> methods;
  methods.reserve(num_methods);
  while (num_methods-- > 0) {
    methods.emplace_back(read_function());
  }

  return {std::move(class_name), std::move(methods)};
}
}  // namespace meta

#endif  //! META_H
