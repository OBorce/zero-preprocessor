#ifndef STD_AST_H
#define STD_AST_H

#include <algorithm>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <heap_obj.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

namespace std_parser::rules::ast {

struct SourceLocation {
  uint16_t row = 0;
  uint16_t col = 0;
};

using Type_ = std::vector<std::string>;

struct Type;
struct LiteralExpression;

struct TemplateTypes {
  std::vector<std::variant<Type, LiteralExpression>> template_types;
};

struct unqulified_type {
  Type_ name;
  TemplateTypes template_types;
};

struct UnqulifiedType {
  std::vector<unqulified_type> type;
};

struct Literal {
  std::variant<std::vector<int64_t>, double, char, std::string> lit;
};

enum class TypeQualifier {
  Inline,
  Static,
  Const,
  Constexpr,
  L_Ref,
  R_Ref,
  Pointer
};

enum class MethodQualifier { NONE, L_REF, R_REF };

struct Type {
  std::vector<TypeQualifier> left_qualifiers;
  UnqulifiedType type;
  std::vector<TypeQualifier> right_qualifiers;

  bool is_lvalue_reference() const {
    return not right_qualifiers.empty() and
           right_qualifiers.back() == TypeQualifier::L_Ref;
  }

  bool is_lvalue_reference_to_const() const {
    return is_lvalue_reference() and
           ((right_qualifiers.size() > 1 and
             right_qualifiers[right_qualifiers.size() - 2] ==
                 TypeQualifier::Const) or
            (not left_qualifiers.empty() and
             std::find(left_qualifiers.begin(), left_qualifiers.end(),
                       TypeQualifier::Const) != left_qualifiers.end()));
  }

  bool is_rvalue_reference() const {
    return not right_qualifiers.empty() and
           right_qualifiers.back() == TypeQualifier::R_Ref;
  }

  bool is_reference() const {
    return is_lvalue_reference() or is_rvalue_reference();
  }

  bool is_value() const { return not is_reference() and not is_pointer(); }

  bool is_pointer() const {
    // FIXME: not correct since last can be const
    return not right_qualifiers.empty() and
           right_qualifiers.back() == TypeQualifier::Pointer;
  }
};

struct Expression;

struct var;

struct params {
  std::vector<var> parameters;
};

struct Params {
  std::vector<var> parameters;

  SourceLocation loc;
};

struct TemplateParameter {
  Type_ type;
  bool is_variadic;
  std::string name;
};

using TemplateParameters = std::vector<TemplateParameter>;

struct function_signature_old {
  TemplateParameters template_parameters;
  bool is_constexpr;
  Type return_type;
  std::string name;
  params parameters;
  bool is_noexcept;

  SourceLocation loc;
};

struct UserDeductionGuide {
  TemplateParameters template_parameters;
  std::string name;
  params parameters;
  Type return_type;

  SourceLocation loc;
};

struct FunctionDeclaration {
  TemplateParameters template_parameters;
  bool is_constexpr;
  Type return_type;
  std::string name;
  params parameters;
  bool is_noexcept;

  SourceLocation loc;
};

struct operator_signature {
  TemplateParameters template_parameters;
  bool is_constexpr;
  bool is_virtual;
  Type return_type;
  params parameters;
  bool is_noexcept;
  bool is_pure_virtual;

  SourceLocation loc;
};

struct method_signature {
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

  SourceLocation loc;
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

  SourceLocation loc;
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
  std::optional<std::vector<TemplateParameter>> template_parameters;
  class_type type;
  std::string name;
  TemplateTypes specialization;
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
  Eq,
  Comma
};

struct VariableExpression {
  UnqulifiedType expression;

  /**
   * Return true if no :: in name and not template params <>
   */
  bool has_name_only() const {
    return expression.type.size() == 1 and
           expression.type.front().name.size() == 1 and
           expression.type.front().template_types.template_types.empty();
  }

  std::string const& get_single_name() const {
    return expression.type.front().name.front();
  }
};

struct LiteralExpression {
  Literal lit;
  UnqulifiedType type;
};

struct RoundExpression;
struct CurlyExpression;
struct Lambda;

using ExpressionVariant = std::variant<VariableExpression,
                                       LiteralExpression,
                                       RoundExpression,
                                       CurlyExpression,
                                       Lambda>;

struct RoundExpression {
  std::optional<UnqulifiedType> functor;

  std::vector<ExpressionVariant> expressions;
  std::vector<Operator> operators;

  SourceLocation loc;

  RoundExpression() = default;
  RoundExpression(UnqulifiedType&& type) : functor{std::move(type)} {}

  bool is_begin() const { return expressions.size() <= operators.size(); }
};

struct CurlyExpression {
  std::optional<UnqulifiedType> type;

  std::vector<ExpressionVariant> expressions;
  std::vector<Operator> operators;

  SourceLocation loc;

  CurlyExpression() = default;
  CurlyExpression(UnqulifiedType&& type) : type{std::move(type)} {}

  bool is_begin() const { return expressions.size() <= operators.size(); }
};

class Scope;
struct Statement;
struct ReturnStatement;
struct Vars;
struct IfStatement;

using StatementVariant =
    std::variant<Vars, Scope, Statement, IfStatement, ReturnStatement>;

enum class LambdaState { Capture, Template, Arguments, Body };
struct Lambda {
  LambdaState state = LambdaState::Capture;

  std::vector<StatementVariant> statements;

  SourceLocation loc;
};

struct Expression {
  std::vector<ExpressionVariant> expressions;
  std::vector<Operator> operators;

  SourceLocation loc;

  Expression() = default;

  Expression(VariableExpression&& type) : expressions{std::move(type)} {}
  Expression(LiteralExpression&& type) : expressions{std::move(type)} {}

  explicit Expression(RoundExpression&& e) : expressions{std::move(e)} {}
  explicit Expression(CurlyExpression&& e) : expressions{std::move(e)} {}
  explicit Expression(Lambda&& e) : expressions{std::move(e)} {}

  operator bool() const { return not expressions.empty(); }

  // TODO: think of a better name
  // add contract that expressions size should never be < operators size
  bool is_begin() const { return expressions.size() <= operators.size(); }

  bool empty() const { return expressions.empty(); }
};

struct var {
  Type type;
  std::string name;
  SourceLocation loc;

  Expression init;
};

/**
 * Init is for the var initialization part,
 * Next is for after next variable
 */
enum class VarDefinition { Init, Next };
struct Vars {
  VarDefinition state = VarDefinition::Init;
  std::vector<var> variables;

  SourceLocation loc;

  Vars() = default;
  Vars(var&& v) : variables{std::move(v)} {}

  void add_var(std::string&& name, SourceLocation l) {
    auto type = variables.front().type;
    variables.push_back(var{std::move(type), std::move(name), l, {}});
  }
};

struct VarStatement : public Vars {};

/**
 * Used as a bridge between an UnqulifiedType and the Expression
 * Needed for the value_expression and expression rules
 */
struct ValueExpression {
  UnqulifiedType type;
  std::variant<RoundExpression, CurlyExpression, VariableExpression> exp;

  // TODO: maybe constraint it to variants that contain Expression
  template<class... Ts>
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

  SourceLocation loc;
};

struct ReturnStatement {
  Expression expression;

  SourceLocation loc;
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

  SourceLocation loc;

  Enumeration(enum_&& e) :
      type{e.type}, name{std::move(e.name)}, as{std::move(e.as)} {}

  void set_enumerators(std::vector<std::string>&& e) { enumerators = std::move(e); }

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

  SourceLocation loc;

  // NOTE: used in the generation of meta classes
  // contains everything inside the brackets of the function
  std::string body;

  Function() = default;

  Function(function_signature_old&& fun) :
      template_parameters{std::move(fun.template_parameters)},
      is_constexpr{fun.is_constexpr},
      return_type{std::move(fun.return_type)},
      name{std::move(fun.name)},
      parameters{std::move(fun.parameters)},
      is_noexcept{fun.is_noexcept},
      loc{fun.loc} {}

  Function(FunctionDeclaration&& fun) :
      template_parameters{std::move(fun.template_parameters)},
      is_constexpr{fun.is_constexpr},
      return_type{std::move(fun.return_type)},
      name{std::move(fun.name)},
      parameters{std::move(fun.parameters)},
      is_noexcept{fun.is_noexcept},
      loc{fun.loc} {}

  Function(method_signature&& fun) :
      template_parameters{std::move(fun.template_parameters)},
      is_constexpr{fun.is_constexpr},
      is_virtual{fun.is_virtual},
      return_type{std::move(fun.return_type)},
      name{std::move(fun.name)},
      parameters{std::move(fun.parameters)},
      is_const{fun.is_const},
      qualifier{fun.qualifier},
      is_noexcept{fun.is_noexcept},
      is_override{fun.is_override},
      is_pure_virtual{fun.is_pure_virtual},
      loc{fun.loc} {}

  Function(constructor&& fun) :
      template_parameters{std::move(fun.template_parameters)},
      is_constexpr{fun.is_constexpr},
      is_virtual{fun.is_virtual},
      constructor_type{fun.type},
      return_type{},
      name{std::move(fun.name)},
      parameters{std::move(fun.parameters)},
      is_noexcept{fun.is_noexcept},
      is_pure_virtual{fun.is_pure_virtual},
      loc{fun.loc} {}

  Function(operator_signature&& fun) :
      template_parameters{std::move(fun.template_parameters)},
      is_constexpr{fun.is_constexpr},
      return_type{std::move(fun.return_type)},
      // TODO: need operator name
      name{"op"},
      parameters{std::move(fun.parameters)},
      is_noexcept{fun.is_noexcept},
      is_pure_virtual{fun.is_pure_virtual},
      loc{fun.loc} {}
};

struct Class {
  class_type type;
  std::string name;
  std::optional<TemplateParameters> template_parameters;
  TemplateTypes specialization;
  access_modifier state;

  std::vector<UnqulifiedType> public_bases;
  std::vector<UnqulifiedType> protected_bases;
  std::vector<UnqulifiedType> private_bases;

  // can have same class names i.e. specializations
  std::map<std::string, std::list<Class>> classes;
  std::map<std::string, Enumeration> enums;

  std::vector<Function> public_methods;
  std::vector<Function> protected_methods;
  std::vector<Function> private_methods;
  std::vector<Function> unspecified_methods;

  std::vector<var> public_members;
  std::vector<var> protected_members;
  std::vector<var> private_members;
  std::vector<var> unspecified_members;

  SourceLocation loc;

  Class(class_or_struct&& cs) :
      type{cs.type},
      name{std::move(cs.name)},
      template_parameters{std::move(cs.template_parameters)},
      specialization{std::move(cs.specialization)} {
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

  bool is_templated() { return template_parameters.has_value(); }

  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes[name].push_back(std::move(class_or_struct));
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
  SourceLocation loc;
  std::vector<StatementVariant> statements;

  void add_class(Class&& class_or_struct) {
    auto name = class_or_struct.name;
    classes.emplace(name, std::move(class_or_struct));
  }

  auto get_class(const std::string& name) const { return classes.at(name); }
};

struct IfStatement {
  enum class State { Begin, IfExpression, CloseBracket, ElseIf, Done };

  State state = State::Begin;

  Vars if_init;
  Expression if_expression;

  HeapObj<StatementVariant> body;

  std::vector<IfStatement> else_if_statements;

  HeapObj<StatementVariant> else_body;

  SourceLocation loc;
};

class Namespace {
  using CodeFragment = std::
      variant<Class, Enumeration, Function, var, Namespace, UserDeductionGuide>;

  std::string name;
  std::vector<CodeFragment> code_fragments;

 public:
  SourceLocation loc;
  Namespace(const std::string& name) : name{name} {}
  Namespace(std::string&& name) : name{std::move(name)} {}

  auto const& get_all_code_fragments() const { return code_fragments; }

  void add_user_deduction_guide(UserDeductionGuide&& udg) {
    code_fragments.push_back(std::move(udg));
  }

  void add_class(Class&& class_or_struct) {
    code_fragments.push_back(std::move(class_or_struct));
  }

  void add_enum(Enumeration&& enumeration) {
    code_fragments.push_back(std::move(enumeration));
  }

  void add_function(Function&& fun) { code_fragments.push_back(std::move(fun)); }

  Function const* find_function(std::string_view name) const {
    auto found = std::find_if(
        code_fragments.cbegin(), code_fragments.cend(), [name](auto& fragment) {
          if (not std::holds_alternative<Function>(fragment)) {
            return false;
          }
          auto& fun = std::get<Function>(fragment);
          return fun.name == name;
        });

    if (found != code_fragments.cend()) {
      auto& fun = std::get<Function>(*found);
      return std::addressof(fun);
    }

    return {};
  }

  void add_variable(var&& var) { code_fragments.push_back(std::move(var)); }

  void add_variables(std::vector<var>& vars) {
    for (auto& var : vars) {
      add_variable(std::move(var));
    }
    vars.clear();
  }

  void add_namespace(Namespace&& n) { code_fragments.push_back(std::move(n)); }
};
} // namespace std_parser::rules::ast

BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateTypes, template_types)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::unqulified_type,
                          name,
                          template_types)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::UnqulifiedType, type)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::Type,
                          left_qualifiers,
                          type,
                          right_qualifiers)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::Literal, lit)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::var, type, name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::LiteralExpression, lit, type)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::params, parameters)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::TemplateParameter,
                          type,
                          is_variadic,
                          name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::ValueExpression, type, exp)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::function_signature_old,
                          template_parameters,
                          is_constexpr,
                          return_type,
                          name,
                          parameters,
                          is_noexcept)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::UserDeductionGuide,
                          template_parameters,
                          name,
                          parameters,
                          return_type)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::FunctionDeclaration,
                          template_parameters,
                          is_constexpr,
                          return_type,
                          name)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::method_signature,
                          template_parameters,
                          is_constexpr,
                          is_virtual,
                          return_type,
                          name,
                          parameters,
                          is_const,
                          qualifier,
                          is_noexcept,
                          is_override,
                          is_pure_virtual)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::operator_signature,
                          template_parameters,
                          is_constexpr,
                          is_virtual,
                          return_type,
                          parameters,
                          is_noexcept,
                          is_pure_virtual)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::constructor,
                          template_parameters,
                          is_constexpr,
                          is_virtual,
                          type,
                          name,
                          parameters,
                          is_noexcept,
                          is_pure_virtual)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_inheritance, modifier, type)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_bases, bases)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::enum_, type, name, as)
BOOST_FUSION_ADAPT_STRUCT(std_parser::rules::ast::class_or_struct,
                          template_parameters,
                          type,
                          name,
                          specialization,
                          bases)

#endif // STD_AST_H
