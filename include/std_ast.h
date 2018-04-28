#ifndef STD_AST_H
#define STD_AST_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>

namespace std_parser::rules::ast {
using Type_ = std::vector<std::string>;

struct Type;

struct TemplateTypes {
  std::vector<Type> template_types;
};

struct Type {
  Type_ name;
  TemplateTypes template_types;
};

struct var {
  Type type;
  std::string name;
};

struct params {
  std::vector<var> parameters;
};

struct TemplateParameters {
  std::vector<std::string> template_parameters;

  bool empty() { return template_parameters.empty(); }
};

struct function_signiture {
  TemplateParameters template_parameters;
  Type return_type;
  std::string name;
  params parameters;
};

struct operator_signiture {
  TemplateParameters template_parameters;
  Type return_type;
  params parameters;
};

enum class class_type { CLASS, STRUCT };

struct class_or_struct {
  TemplateParameters template_parameters;
  class_type type;
  std::string name;
};

struct Function {
  TemplateParameters template_parameters;
  Type return_type;
  std::string name;
  params parameters;

  Function(function_signiture&& fun)
      : template_parameters{std::move(fun.template_parameters)},
        return_type{std::move(fun.return_type)},
        name{std::move(fun.name)},
        parameters{std::move(fun.parameters)} {}

  Function(operator_signiture&& fun)
      : template_parameters{std::move(fun.template_parameters)},
        return_type{std::move(fun.return_type)},
        name{"op"},
        parameters{std::move(fun.parameters)} {}
};

struct Class {
  class_type type;
  std::string name;
  TemplateParameters template_parameters;
  enum class Modifier { PUBLIC, PROTECTED, PRIVATE } state;

  std::map<std::string, Class> classes;
  std::unordered_map<std::string, Function> public_methods;
  std::unordered_map<std::string, Function> protected_methods;
  std::unordered_map<std::string, Function> private_methods;

  std::vector<var> public_members;
  std::vector<var> protected_members;
  std::vector<var> private_members;

  Class(class_or_struct&& cs)
      : type{cs.type},
        name{std::move(cs.name)},
        template_parameters{std::move(cs.template_parameters)} {
    switch (type) {
      case class_type::CLASS:
        state = Modifier::PRIVATE;
        break;
      case class_type::STRUCT:
        state = Modifier::PUBLIC;
        break;
    }
  }

  bool is_templated() { return !template_parameters.empty(); }

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
    switch (state) {
      case Modifier::PUBLIC:
        public_members.emplace_back(std::move(var));
        break;
      case Modifier::PROTECTED:
        protected_members.emplace_back(std::move(var));
        break;
      case Modifier::PRIVATE:
        private_members.emplace_back(std::move(var));
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

  auto const& get_class(const std::string& name) const {
    return classes.at(name);
  }

  auto const& get_namespace(const std::string& name) const {
    return nested_namespaces.at(name);
  }
};
}  // namespace std_parser::rules::ast

BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateTypes, template_types)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::Type, name, template_types)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::var, type, name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::params, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateParameters,
                          template_parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::function_signiture,
                          template_parameters, return_type, name, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::operator_signiture,
                          template_parameters, return_type, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_or_struct,
                          template_parameters, type, name)

#endif  //! STD_AST_H
