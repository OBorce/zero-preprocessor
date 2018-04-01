#ifndef STD_AST_H
#define STD_AST_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>

namespace std_parser::rules::ast {
using Type = std::vector<std::string>;
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

struct Class {
  class_type type;
  std::string name;
  enum class Modifier { PUBLIC, PROTECTED, PRIVATE } state;


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

class Namespace {
  std::string name;
  std::unordered_map<std::string, Class> classes;
  std::unordered_map<std::string, Function> functions;
  std::unordered_map<std::string, var> variables;
  std::map<std::string, Namespace> nested_namespaces;

 public:
  Namespace(const std::string& name) : name{name} {}

  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes.emplace(name, std::move(class_or_struct));
  }

  void add_function(function_signiture&& fun) {
    auto name = fun.name;
    functions.emplace(name, std::move(fun));
  }

  void add_variable(var& var) { variables.emplace(var.name, var); }

  void add_namespace(Namespace&& n) {
    auto name = n.name;
    nested_namespaces.emplace(name, std::move(n));
  }

  auto get_class(const std::string& name) const { return classes.at(name); }

  auto get_namespace(const std::string& name) const {
    return nested_namespaces.at(name);
  }
};
}  // namespace std_parser::rules::ast

BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::var, type, name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::function_signiture,
                          return_type, name, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_or_struct, type, name)

#endif  //! STD_AST_H
