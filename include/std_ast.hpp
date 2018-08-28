#ifndef STD_AST_H
#define STD_AST_H

#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <boost/fusion/include/adapt_struct.hpp>

namespace std_parser::rules::ast {
using Type_ = std::vector<std::string>;

struct Type;

struct TemplateTypes {
  std::vector<Type> template_types;
};

struct UnqulifiedType {
  Type_ name;
  TemplateTypes template_types;
};

enum class TypeQualifier { Const, Constexpr, L_Ref, R_Ref, Pointer };

enum class MethodQualifier { NONE, L_REF, R_REF };

struct Type {
  std::vector<TypeQualifier> left_qualifiers;
  UnqulifiedType type;
  std::vector<TypeQualifier> right_qualifiers;
};

struct var {
  Type type;
  std::string name;
};

/**
 * Init is for the var initialization part,
 * Next is for after next variabl
 */
enum class VarDefinition { Init, Next };
struct Vars {
  VarDefinition state = VarDefinition::Init;
  std::vector<var> variables;

  Vars(var&& v) : variables{std::move(v)} {}

  void add_var(std::string&& name) {
    auto type = variables.front().type;
    variables.emplace_back(var{std::move(type), std::move(name)});
  }
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
  bool is_constexpr;
  Type return_type;
  std::string name;
  params parameters;
  bool is_noexcept;
};

struct operator_signiture {
  TemplateParameters template_parameters;
  bool is_constexpr;
  bool is_virtual;
  Type return_type;
  params parameters;
  bool is_noexcept;
  bool is_pure_virtual;
};

struct method_signiture {
  TemplateParameters template_parameters;
  bool is_constexpr;
  bool is_virtual;
  Type return_type;
  std::string name;
  params parameters;
  bool is_const;
  MethodQualifier qualifier = MethodQualifier::NONE;
  bool is_noexcept;
  bool is_override;
  bool is_pure_virtual;
};

enum class Constructor { CONSTRUCTOR, DESTRUCTOR, NOTHING };

struct constructor {
  TemplateParameters template_parameters;
  bool is_constexpr;
  bool is_virtual;
  Constructor type = Constructor::CONSTRUCTOR;
  std::string name;
  params parameters;
  bool is_noexcept;
  bool is_pure_virtual;
};

enum class class_type { CLASS, STRUCT, META_CLASS };

enum class access_modifier { PUBLIC, PROTECTED, PRIVATE, UNSPECIFIED };

struct class_inheritance {
  access_modifier modifier = access_modifier::PUBLIC;
  UnqulifiedType type;
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

enum class Operator {
  PlusEq,
  Plus,
  ArrowDeref,
  Arrow,
  MinusEq,
  Minus,
  DotDeref,
  Dot,
  MultiplyEq,
  Multiply,
  DivideEq,
  Divide,
  ModuleEq,
  Module,
  RShiftEq,
  RShift,
  GtEq,
  Gt,
  LShiftEq,
  LShift,
  LtEq,
  Lt,
  And,
  BitAndEq,
  BitAnd,
  Or,
  BitOrEq,
  BitOr,
  TildeEq,
  Tilde,
  BitXorEq,
  BitXor,
  NotEq,
  Not,
  EqEq,
  Eq
};

struct VariableExpression {
  UnqulifiedType expression;
};

struct LiteralExpression {
  // TODO: add the literal
};

struct RoundExpression;
struct CurlyExpression;
struct Lambda;

using ExpressionVariant = std::variant<VariableExpression, LiteralExpression,
                                       RoundExpression, CurlyExpression, Lambda>;

struct RoundExpression {
  std::optional<UnqulifiedType> functor;

  std::vector<ExpressionVariant> expressions;
  std::vector<Operator> operators;

  RoundExpression() = default;
  RoundExpression(UnqulifiedType&& type) : functor{std::move(type)} {}

  bool is_begin() const { return expressions.size() <= operators.size(); }
};

struct CurlyExpression {
  std::optional<UnqulifiedType> type;

  std::vector<ExpressionVariant> expressions;
  std::vector<Operator> operators;

  CurlyExpression() = default;
  CurlyExpression(UnqulifiedType&& type) : type{std::move(type)} {}

  bool is_begin() const { return expressions.size() <= operators.size(); }
};

class Scope;
struct Statement;
struct ReturnStatement;

using StatementVariant = std::variant<Vars, Scope, Statement, ReturnStatement>;

enum class LambdaState { Capture, Template, Arguments, Body };
struct Lambda {
  LambdaState state = LambdaState::Capture;

  std::vector<StatementVariant> statements;
};

struct Expression {
  std::vector<ExpressionVariant> expressions;
  std::vector<Operator> operators;

  Expression() = default;

  Expression(VariableExpression&& type) {
    expressions.push_back(std::move(type));
  }

  Expression(LiteralExpression&& type) {
    expressions.emplace_back(std::move(type));
  }

  explicit Expression(RoundExpression&& e) : expressions{std::move(e)} {}
  explicit Expression(CurlyExpression&& e) : expressions{std::move(e)} {}
  explicit Expression(Lambda&& e) : expressions{std::move(e)} {}

  // TODO: think of a better name
  // add contract that expressions size should never be < operators size
  bool is_begin() const { return expressions.size() <= operators.size(); }
};

/**
 * Used as a bridge between an UnqulifiedType and the Expression
 * Needed for the value_expression and expression rules
 */
struct ValueExpression {
  UnqulifiedType type;
  std::variant<RoundExpression, CurlyExpression, VariableExpression> exp;

  // TODO: maybe constraint it to variants that contain Expression
  template <class... Ts>
  operator std::variant<Ts...>() {
    return std::visit(
        [this](auto& exp) {
          return std::variant<Ts...>(
              std::remove_reference_t<decltype(exp)>{std::move(type)});
        },
        exp);
  }
};

struct Statement {
  Expression expression;
};

struct ReturnStatement {
  Expression expression;
};

enum class IfExpressionState { Begin, Expression, Done };
struct IfExpression {
  IfExpressionState state = IfExpressionState::Begin;
};

enum class EnumType { ENUM, ENUM_CLASS };

struct enum_ {
  EnumType type = EnumType::ENUM;
  std::string name;
  Type_ as{"int"};
};

struct Enumeration {
  EnumType type = EnumType::ENUM;
  std::string name;
  Type_ as;
  std::vector<std::string> enumerators;

  Enumeration(enum_&& e)
      : type{e.type}, name{std::move(e.name)}, as{std::move(e.as)} {}

  void set_enumerators(std::vector<std::string>&& e) {
    enumerators = std::move(e);
  }

  bool is_scoped() { return type == EnumType::ENUM_CLASS; }
};


struct Function {
  TemplateParameters template_parameters;
  bool is_constexpr = false;
  bool is_virtual = false;
  Constructor constructor_type = Constructor::NOTHING;
  Type return_type;
  std::string name;
  params parameters;
  bool is_const = false;
  MethodQualifier qualifier = MethodQualifier::NONE;
  bool is_noexcept = false;
  bool is_override = false;
  bool is_pure_virtual = false;

  std::vector<StatementVariant> statements;

  // NOTE: used in the generation of meta classes
  // contains everything inside the brackets of the function
  std::string body;

  Function() = default;

  Function(function_signiture&& fun)
      : template_parameters{std::move(fun.template_parameters)},
        is_constexpr{fun.is_constexpr},
        return_type{std::move(fun.return_type)},
        name{std::move(fun.name)},
        parameters{std::move(fun.parameters)},
        is_noexcept{fun.is_noexcept} {}

  Function(method_signiture&& fun)
      : template_parameters{std::move(fun.template_parameters)},
        is_constexpr{fun.is_constexpr},
        is_virtual{fun.is_virtual},
        return_type{std::move(fun.return_type)},
        name{std::move(fun.name)},
        parameters{std::move(fun.parameters)},
        is_const{fun.is_const},
        qualifier{fun.qualifier},
        is_noexcept{fun.is_noexcept},
        is_override{fun.is_override},
        is_pure_virtual{fun.is_pure_virtual} {}

  Function(constructor&& fun)
      : template_parameters{std::move(fun.template_parameters)},
        is_constexpr{fun.is_constexpr},
        is_virtual{fun.is_virtual},
        constructor_type{fun.type},
        return_type{},
        name{std::move(fun.name)},
        parameters{std::move(fun.parameters)},
        is_noexcept{fun.is_noexcept},
        is_pure_virtual{fun.is_pure_virtual} {}

  Function(operator_signiture&& fun)
      : template_parameters{std::move(fun.template_parameters)},
        is_constexpr{fun.is_constexpr},
        return_type{std::move(fun.return_type)},
        // TODO: need operator name
        name{"op"},
        parameters{std::move(fun.parameters)},
        is_noexcept{fun.is_noexcept},
        is_pure_virtual{fun.is_pure_virtual} {}
};

struct Class {
  class_type type;
  std::string name;
  TemplateParameters template_parameters;
  access_modifier state;

  std::vector<UnqulifiedType> public_bases;
  std::vector<UnqulifiedType> protected_bases;
  std::vector<UnqulifiedType> private_bases;

  std::map<std::string, Class> classes;
  std::map<std::string, Enumeration> enums;

  std::vector<Function> public_methods;
  std::vector<Function> protected_methods;
  std::vector<Function> private_methods;
  std::vector<Function> unspecified_methods;

  std::vector<var> public_members;
  std::vector<var> protected_members;
  std::vector<var> private_members;
  std::vector<var> unspecified_members;

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
      case class_type::META_CLASS:
        state = access_modifier::UNSPECIFIED;
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
        case access_modifier::UNSPECIFIED:
          break;
      }
    }
  }

  void set_access_modifier(access_modifier mod) { state = mod; }

  bool is_templated() { return !template_parameters.empty(); }

  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes.emplace(name, std::move(class_or_struct));
  }

  void add_enum(Enumeration&& enumeration) {
    auto name = enumeration.name;
    enums.emplace(name, std::move(enumeration));
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
      case access_modifier::UNSPECIFIED:
        unspecified_methods.emplace_back(std::move(fun));
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
      case access_modifier::UNSPECIFIED:
        unspecified_members.emplace_back(std::move(var));
        break;
    }
  }

  void add_variables(std::vector<var>& vars) {
    auto& current = [this]() -> auto& {
      switch (state) {
        case access_modifier::PUBLIC:
          return public_members;
        case access_modifier::PROTECTED:
          return protected_members;
        case access_modifier::PRIVATE:
          return private_members;
        default:
          return unspecified_members;
      }
    }
    ();

    current.insert(current.end(), std::make_move_iterator(vars.begin()),
                   std::make_move_iterator(vars.end()));
    vars.clear();
  }
};

class Scope {
  std::unordered_map<std::string, Class> classes;
  std::unordered_map<std::string, var> variables;

 public:
  std::vector<StatementVariant> statements;

  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes.emplace(name, std::move(class_or_struct));
  }

  auto get_class(const std::string& name) const { return classes.at(name); }
};

class Namespace {
  std::string name;
  std::unordered_map<std::string, Class> classes;
  std::unordered_map<std::string, Enumeration> enums;
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

  void add_enum(Enumeration&& enumeration) {
    auto name = enumeration.name;
    enums.emplace(name, std::move(enumeration));
  }

  void add_function(Function&& fun) {
    auto name = fun.name;
    functions.emplace(name, std::move(fun));
  }

  void add_variable(var&& var) {
    auto name = var.name;
    variables.emplace(name, std::move(var));
  }

  void add_variables(std::vector<var>& vars) {
    for (auto& var : vars) {
      add_variable(std::move(var));
    }
    vars.clear();
  }

  void add_namespace(Namespace&& n) {
    auto name = n.name;
    nested_namespaces.emplace(name, std::move(n));
  }

  auto const& get_all_classes() const { return classes; }

  auto const& get_class(const std::string& name) const {
    return classes.at(name);
  }

  auto const& get_namespace(const std::string& name) const {
    return nested_namespaces.at(name);
  }
};
}  // namespace std_parser::rules::ast

BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateTypes, template_types)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::UnqulifiedType, name,
                          template_types)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::Type, left_qualifiers, type,
                          right_qualifiers)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::var, type, name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::params, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateParameter, type, name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateParameters,
                          template_parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::ValueExpression, type, exp)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::function_signiture,
                          template_parameters, is_constexpr, return_type, name,
                          parameters, is_noexcept)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::method_signiture,
                          template_parameters, is_constexpr, is_virtual,
                          return_type, name, parameters, is_const, qualifier,
                          is_noexcept, is_override, is_pure_virtual)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::operator_signiture,
                          template_parameters, is_constexpr, is_virtual,
                          return_type, parameters, is_noexcept, is_pure_virtual)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::constructor,
                          template_parameters, is_constexpr, is_virtual, type,
                          name, parameters, is_noexcept, is_pure_virtual)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_inheritance, modifier,
                          type)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_bases, bases)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::enum_, type, name, as)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_or_struct,
                          template_parameters, type, name, bases)

#endif  // STD_AST_H
