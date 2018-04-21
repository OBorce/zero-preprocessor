#ifndef STD_AST_H
#define STD_AST_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>

namespace std_parser::rules::ast {
using Type_ = std::vector<std::string>;

struct Type {
  Type_ name;
};

struct Ints {
  std::vector<int> ints;
  char c;
};

struct var {
  Type type;
  std::string name;
};

struct function_signiture {
  Type return_type;
  std::string name;
  std::vector<var> parameters;
};

enum class class_type { CLASS, STRUCT };

struct class_or_struct {
  class_type type;
  std::string name;
};

struct Function {
  Type return_type;
  std::string name;
  std::vector<var> parameters;

  Function(function_signiture&& fun)
      : return_type{std::move(fun.return_type)},
        name{std::move(fun.name)},
        parameters{std::move(fun.parameters)} {}
};

struct Class {
  class_type type;
  std::string name;
  enum class Modifier { PUBLIC, PROTECTED, PRIVATE } state;

  std::map<std::string, Class> classes;
  std::unordered_map<std::string, Function> public_methods;
  std::unordered_map<std::string, Function> protected_methods;
  std::unordered_map<std::string, Function> private_methods;

  std::unordered_map<std::string, var> public_members;
  std::unordered_map<std::string, var> protected_members;
  std::unordered_map<std::string, var> private_members;

  Class(class_or_struct&& cs) : type{cs.type}, name{std::move(cs.name)} {
    switch (type) {
      case class_type::CLASS:
        state = Modifier::PRIVATE;
        break;
      case class_type::STRUCT:
        state = Modifier::PUBLIC;
        break;
    }
  }

  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes.emplace(name, std::move(class_or_struct));
  }

  void add_function(Function&& fun) {
    auto name = fun.name;
    switch (state) {
      case Modifier::PUBLIC:
        public_methods.emplace(name, std::move(fun));
        break;
      case Modifier::PROTECTED:
        protected_methods.emplace(name, std::move(fun));
        break;
      case Modifier::PRIVATE:
        private_methods.emplace(name, std::move(fun));
        break;
    }
  }

  void add_variable(var&& var) {
    auto name = var.name;
    switch (state) {
      case Modifier::PUBLIC:
        public_members.emplace(name, std::move(var));
        break;
      case Modifier::PROTECTED:
        protected_members.emplace(name, std::move(var));
        break;
      case Modifier::PRIVATE:
        private_members.emplace(name, std::move(var));
        break;
    }
  }
};

class Scope {
  std::unordered_map<std::string, Class> classes;
  std::unordered_map<std::string, var> variables;

 public:
  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes.emplace(name, std::move(class_or_struct));
  }

  void add_variable(var&& var) {
    auto name = var.name;
    variables.emplace(name, std::move(var));
  }

  auto get_class(const std::string& name) const { return classes.at(name); }
};

class Namespace {
  std::string name;
  std::unordered_map<std::string, Class> classes;
  std::unordered_map<std::string, Function> functions;
  std::unordered_map<std::string, var> variables;
  std::map<std::string, Namespace> nested_namespaces;

 public:
  Namespace(const std::string& name) : name{name} {}
  Namespace(std::string&& name) : name{std::move(name)} {}

  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes.emplace(name, std::move(class_or_struct));
  }

  void add_function(Function&& fun) {
    auto name = fun.name;
    functions.emplace(name, std::move(fun));
  }

  void add_variable(var&& var) {
    auto name = var.name;
    variables.emplace(name, std::move(var));
  }

  void add_namespace(Namespace&& n) {
    auto name = n.name;
    nested_namespaces.emplace(name, std::move(n));
  }

  auto const& get_class(const std::string& name) const { return
classes.at(name); }

  auto const& get_namespace(const std::string& name) const {
    return nested_namespaces.at(name);
  }
};
}  // namespace std_parser::rules::ast

BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::Type, name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::Ints, ints, c)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::var, type, name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::function_signiture, return_type, name, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_or_struct, type, name)

#endif  //! STD_AST_H
