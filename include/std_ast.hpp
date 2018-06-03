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

  std::string to_string() const {
    std::string type_out;
    type_out.reserve(30);
    for (auto& t : name) {
      type_out += t;
      type_out += "::";
    }
    type_out.pop_back();
    type_out.pop_back();

    auto& templates = template_types.template_types;
    if (!templates.empty()) {
      type_out += '<';
      for (auto& type : templates) {
        type_out += type.to_string();
      }
      type_out += '>';
    }

    return type_out;
  }
};

struct var {
  Type type;
  std::string name;
};

struct params {
  std::vector<var> parameters;
};

struct TemplateParameter {
  Type_ type;
  std::string name;
};

struct TemplateParameters {
  std::vector<TemplateParameter> template_parameters;

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

enum class access_modifier { PUBLIC, PROTECTED, PRIVATE };

struct class_inheritance {
  access_modifier modifier = access_modifier::PUBLIC;
  Type type;
};

struct class_bases {
  std::vector<class_inheritance> bases;
};

struct class_or_struct {
  TemplateParameters template_parameters;
  class_type type;
  std::string name;
  class_bases bases;
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
  access_modifier state;

  std::vector<Type> public_bases;
  std::vector<Type> protected_bases;
  std::vector<Type> private_bases;

  std::map<std::string, Class> classes;
  std::vector<Function> public_methods;
  std::vector<Function> protected_methods;
  std::vector<Function> private_methods;

  std::vector<var> public_members;
  std::vector<var> protected_members;
  std::vector<var> private_members;

  Class(class_or_struct&& cs)
      : type{cs.type},
        name{std::move(cs.name)},
        template_parameters{std::move(cs.template_parameters)} {
    switch (type) {
      case class_type::CLASS:
        state = access_modifier::PRIVATE;
        break;
      case class_type::STRUCT:
        state = access_modifier::PUBLIC;
        break;
    }

    for (auto& base : cs.bases.bases) {
      switch (base.modifier) {
        case access_modifier::PUBLIC:
          public_bases.emplace_back(std::move(base.type));
          break;
        case access_modifier::PROTECTED:
          protected_bases.emplace_back(std::move(base.type));
          break;
        case access_modifier::PRIVATE:
          private_bases.emplace_back(std::move(base.type));
          break;
      }
    }
  }

  Class(std::string&& name)
      : type{class_type::STRUCT},
        name{std::move(name)}
  // TODO: add templates
  // template_parameters{std::move(cs.template_parameters)}
  {
    switch (type) {
      case class_type::CLASS:
        state = access_modifier::PRIVATE;
        break;
      case class_type::STRUCT:
        state = access_modifier::PUBLIC;
        break;
    }
  }

  void set_access_modifier(access_modifier mod) { state = mod; }

  bool is_templated() { return !template_parameters.empty(); }

  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes.emplace(name, std::move(class_or_struct));
  }

  void add_function(Function&& fun) {
    switch (state) {
      case access_modifier::PUBLIC:
        public_methods.emplace_back(std::move(fun));
        break;
      case access_modifier::PROTECTED:
        protected_methods.emplace_back(std::move(fun));
        break;
      case access_modifier::PRIVATE:
        private_methods.emplace_back(std::move(fun));
        break;
    }
  }

  void add_variable(var&& var) {
    switch (state) {
      case access_modifier::PUBLIC:
        public_members.emplace_back(std::move(var));
        break;
      case access_modifier::PROTECTED:
        protected_members.emplace_back(std::move(var));
        break;
      case access_modifier::PRIVATE:
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
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateParameter, type, name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateParameters,
                          template_parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::function_signiture,
                          template_parameters, return_type, name, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::operator_signiture,
                          template_parameters, return_type, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_inheritance, modifier,
                          type)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_bases, bases)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_or_struct,
                          template_parameters, type, name, bases)

#endif  //! STD_AST_H
