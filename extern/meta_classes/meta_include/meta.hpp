#ifndef META_H
#define META_H

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

static struct {
  void require(bool b, std::string_view msg) {
    if (!b) {
      std::cout << -1 << std::endl;
      std::cout << msg.size() << std::endl;
      std::cout << msg << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  template <class T>
  void require(bool b, std::string_view msg, T& t) {
    if (!b) {
      std::cout << -1 << std::endl;
      std::string msg_with_loc;
      msg_with_loc += ':';
      auto loc = t.loc;
      msg_with_loc += std::to_string(loc.row);
      msg_with_loc += ':';
      msg_with_loc += std::to_string(loc.col);
      msg_with_loc += ": ";
      msg_with_loc += msg;
      std::cout << msg_with_loc.size() << std::endl;
      std::cout << msg_with_loc << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  void error(std::string_view msg) {
    std::cout << -1 << std::endl;
    std::cout << msg.size() << std::endl;
    std::cout << msg << std::endl;
    std::exit(EXIT_FAILURE);
  }
} compiler;

namespace meta {
class type;

namespace detail {
enum class Access { PUBLIC, PROTECTED, PRIVATE, UNSPECIFIED };

enum class TypeQualifier { Const, Constexpr, L_Ref, R_Ref, Pointer };

std::string to_string(TypeQualifier q) {
  switch (q) {
    case TypeQualifier::Const:
      return "const";
    case TypeQualifier::Constexpr:
      return "constexpr";
    case TypeQualifier::L_Ref:
      return "&";
    case TypeQualifier::R_Ref:
      return "&&";
    case TypeQualifier::Pointer:
      return "*";
  }

  return "";
}

std::string to_string(Access a) {
  switch (a) {
    case Access::PUBLIC:
      return "public: ";
    case Access::PROTECTED:
      return "protected: ";
    case Access::PRIVATE:
      return "private: ";
    default:
      return "";
  }
}

struct SourceLocation {
  uint16_t row = 0;
  uint16_t col = 0;
};

struct TemplateParameter {
  std::string type;
  std::string name;
};

struct CppType {
  std::vector<TypeQualifier> left_qualifiers;
  std::string type;
  std::vector<TypeQualifier> right_qualifiers;

  bool is_auto() { return type == "auto"; }

  void make_constexpr() {
    if (std::find(left_qualifiers.begin(), left_qualifiers.end(),
                  TypeQualifier::Constexpr) != left_qualifiers.end()) {
      left_qualifiers.insert(left_qualifiers.begin(), TypeQualifier::Constexpr);
    }
  }

  std::string to_string() const {
    std::string out;
    out.reserve(50);
    for (auto q : left_qualifiers) {
      out += detail::to_string(q);
      out += ' ';
    }
    out += type;
    for (auto q : right_qualifiers) {
      out += ' ';
      out += detail::to_string(q);
    }
    return out;
  }
};

struct Param {
  CppType type;
  std::string name;
};

struct Var {
  CppType var_type;
  std::string name;
  std::vector<std::string> init;
  Access access = Access::UNSPECIFIED;
  SourceLocation loc;

  auto const& type() { return var_type; }

  bool has_access() { return access != Access::UNSPECIFIED; }

  bool is_public() { return access == Access::PUBLIC; }

  bool is_protected() { return access == Access::PROTECTED; }

  bool is_private() { return access == Access::PRIVATE; }

  void make_public() { access = Access::PUBLIC; }

  void make_private() { access = Access::PRIVATE; }

  void make_protected() { access = Access::PROTECTED; }

  std::string to_string() {
    std::string  str =  detail::to_string(access) + var_type.to_string() + " " + name;
    if (not init.empty()) {
      str += " = ";
      for(auto& exp : init) {
        str += exp;
        str += ' ';
      }
    }
    str += ';';
    return str;
  }
};

enum class Constructor { CONSTRUCTOR, DESTRUCTOR, NOTHING };
enum class MethodQualifier { NONE, L_REF, R_REF };

struct Function {
  SourceLocation loc;
  CppType return_type;
  bool is_virtual_;
  Constructor constructor_type;
  std::string name;
  std::vector<Param> parameters;
  bool is_const;
  MethodQualifier qualifier;
  bool is_noexcept;
  bool is_override;
  Access access = Access::UNSPECIFIED;
  bool is_pure_virtual = false;
  std::string body;

  bool is_constructor() { return constructor_type == Constructor::CONSTRUCTOR; }

  bool is_destructor() { return constructor_type == Constructor::DESTRUCTOR; }

  bool is_copy() {
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

    if (qualifiers.front() != TypeQualifier::Const) {
      return false;
    }

    if (qualifiers.back() != TypeQualifier::L_Ref) {
      return false;
    }

    // TODO: check for return type or constructor

    return name == param.type;
  }

  bool is_move() {
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

    if (param.right_qualifiers.front() != TypeQualifier::R_Ref) {
      return false;
    }

    // TODO: check for return type or constructor

    return name == param.type;
  }

  bool is_default_ctor() {
    return constructor_type == Constructor::CONSTRUCTOR && parameters.empty();
  }

  bool is_copy_ctor() {
    return is_copy() && constructor_type == Constructor::CONSTRUCTOR;
  }

  bool is_move_ctor() {
    return is_move() && constructor_type == Constructor::CONSTRUCTOR;
  }

  bool is_copy_assignment() {
    return is_copy() && constructor_type == Constructor::NOTHING;
  }

  bool is_move_assignment() {
    return is_move() && constructor_type == Constructor::NOTHING;
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
    is_pure_virtual = true;
    body.clear();
  }

  std::string to_string() {
    std::string s;
    s.reserve(100);
    s += detail::to_string(access);
    if (is_virtual_) {
      s += "virtual ";
    }
    s += return_type.to_string();
    s += ' ';
    if (constructor_type == Constructor::DESTRUCTOR) {
      s += '~';
    }
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

    if (is_noexcept) {
      s += " noexcept";
    }

    if (is_override) {
      s += " override";
    }

    if (is_pure_virtual) {
      s += " = 0";
    }

    if (body.empty()) {
      s += ';';
      s += '\n';
    } else {
      s += '{';
      s += body;
      s += '}';
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

struct Object {
  std::variant<Var, Base, Function> data;

  Object(const Var& v) : data{v} {}
  Object(const Base& b) : data{b} {}
  Object(const Function& b) : data{b} {}
};

struct Type {
  std::string name;
  std::optional<std::vector<TemplateParameter>> template_params;
  std::vector<std::string> template_specialization;
  std::vector<Function> methods;
  std::vector<Var> variables;
  std::vector<Base> bases;
  std::vector<Type> sub_types;
  std::string body;
  SourceLocation loc;

  Type(std::string name): name{std::move(name)} {}

  Type(std::string&& name, std::optional<std::vector<TemplateParameter>>&& tps, std::vector<std::string>&& spec, std::vector<Function>&& m, std::vector<Var>&& v,
       std::vector<Base>&& bases, std::vector<Type>&& sub_types)
      : name{std::move(name)},
        template_params{std::move(tps)},
        template_specialization{std::move(spec)},
        methods{std::move(m)},
        variables{std::move(v)},
        bases{std::move(bases)},
        sub_types{std::move(sub_types)},
        body{} {}

  std::string to_string() {
    std::string content;
    content.reserve(200);
    if (template_params.has_value()) {
      content += "\ntemplate<";
      for(auto& t : *template_params) {
        content += t.type;
        content += ' ';
        content += t.name;
        content += ',';
      }
      if (template_params->empty()) {
        content += '>';
      } else {
        content.back() = '>';
      }
    }
    content += "\nstruct ";
    content += name;
    if (not template_specialization.empty()) {
      content += '<';
      for(auto& t : template_specialization) {
        content += t;
        content += ',';
      }
      content.back() = '>';
    }
    if (not bases.empty()) {
      content += ':';
      for (auto& b : bases) {
        content += b.to_string();
        content += ',';
      }
      content.pop_back();
    }
    content += " {\n";
    if (not variables.empty()) {
      for (auto& v : variables) {
        content += v.to_string();
        content += '\n';
      }
      content.pop_back();
    }
    if (not methods.empty()) {
      for (auto& f : methods) {
        content += f.to_string();
        content += '\n';
      }
      content.pop_back();
    }
    if (not sub_types.empty()) {
      for (auto& t : sub_types) {
        content += t.to_string();
        content += '\n';
      }
      content.pop_back();
    }
    content += body;
    content += "\n};";

    return content;
  }
};
void finalize(meta::type& target);
Type read_type();
}  // namespace detail

type read_type();

class type {
  const std::string class_name;
  std::shared_ptr<detail::Type> internal;

 public:
  type(detail::Type&& type)
      : class_name{type.name},
        internal{std::make_shared<detail::Type>(std::move(type))} {}

  type(std::string name)
      : class_name{name},
        internal{std::make_shared<detail::Type>(name)} {}

  ~type() {
    // NOTE: if there is any generated ( -> ) based content
    // send it for parsing and update our internal state
    if (!internal->body.empty()) {
      enum class ParsedResult { OK, Error };
      std::cout << 1 << std::endl;
      auto str = internal->to_string();
      std::cout << str.size() << std::endl;
      std::cout << str << std::endl;
      int out;
      std::cin >> out;
      auto result = static_cast<ParsedResult>(out);
      if (result == ParsedResult::Error) {
        std::exit(EXIT_FAILURE);
      }

      detail::Type t = detail::read_type();
      *internal = std::move(t);
    }
  }

  auto const& name() const { return class_name; }

  auto const& functions() const { return internal->methods; }

  auto const& bases() const { return internal->bases; }

  auto const& variables() const { return internal->variables; }

  auto members_and_bases() const {
    std::vector<detail::Object> members_and_bases;
    members_and_bases.reserve(internal->bases.size() +
                              internal->variables.size() +
                              internal->methods.size());

    members_and_bases.insert(members_and_bases.end(), internal->bases.begin(),
                             internal->bases.end());

    members_and_bases.insert(members_and_bases.end(),
                             internal->variables.begin(),
                             internal->variables.end());

    members_and_bases.insert(members_and_bases.end(), internal->methods.begin(),
                             internal->methods.end());

    return members_and_bases;
  }

  auto& operator<<(std::string_view s) {
    internal->body += s;
    return *this;
  }

  auto& operator<<(int s) {
    internal->body += std::to_string(s);
    return *this;
  }

  auto& operator<<(detail::CppType const& t) {
    internal->body += t.to_string();
    return *this;
  }

  auto& operator<<(detail::Function& f) {
    internal->methods.push_back(f);
    return *this;
  }

  auto& operator<<(detail::Base& b) {
    internal->bases.push_back(b);
    return *this;
  }

  auto& operator<<(detail::Object& b) {
    if (std::holds_alternative<detail::Base>(b.data)) {
      internal->bases.push_back(std::get<detail::Base>(b.data));
    } else if (std::holds_alternative<detail::Var>(b.data)) {
      internal->variables.push_back(std::get<detail::Var>(b.data));
    } else {
      internal->methods.push_back(std::get<detail::Function>(b.data));
    }

    return *this;
  }

  std::string get_representation() {
    detail::finalize(*this);
    return internal->to_string();
  }
};

namespace detail {
SourceLocation read_loc() {
  uint16_t row, col;
  std::cin >> row >> col;
  return {row, col};
}

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
  auto loc = read_loc();
  std::string name;
  auto type = read_cpp_type();
  int a;
  std::cin >> a;
  Access acc = static_cast<Access>(a);
  std::cin >> name;

  int exps;
  std::cin >> exps;
  std::vector<std::string> init;
  while(exps-- > 0) {
    std::string exp;
    std::cin >> exp;
    init.push_back(std::move(exp));
  }

  return {std::move(type), std::move(name), std::move(init), acc, loc};
}

Function read_function() {
  auto loc = read_loc();
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

  bool is_noexcept;
  std::cin >> is_noexcept;

  bool is_override;
  std::cin >> is_override;

  bool is_pure_virtual;
  std::cin >> is_pure_virtual;

  std::size_t body_size;
  std::cin >> body_size;
  std::string body;
  if (body_size > 0) {
    body.resize(body_size);
    std::cin.read(body.data(), body_size);
  }

  return {loc,
          std::move(return_type),
          is_virtual,
          constructor_type,
          std::move(name),
          std::move(params),
          is_const,
          qualifier,
          is_noexcept,
          is_override,
          acc,
          is_pure_virtual,
          body};
}

Base read_base() {
  std::string name;
  std::cin >> name;
  int a;
  std::cin >> a;
  Access acc = static_cast<Access>(a);

  return {name, acc};
}

void finalize(meta::type& target) {
  for (auto o : target.variables())
    if (!o.has_access()) o.make_private();
  // make data members private by default
  bool __has_declared_dtor = false;
  for (auto f : target.functions()) {
    if (!f.has_access()) f.make_public();
    // make functions public by default
    __has_declared_dtor |= f.is_destructor();
    // and find the destructor
  }
  if (!__has_declared_dtor)  // if no dtor was declared, then
    target << "\npublic: ~" << target.name() << "() { }";
  // make it public nonvirtual by default
}

TemplateParameter read_template_param() {
  std::string type, name;

  std::cin >> type >> name;
  return {std::move(type), std::move(name)};
}

Type read_type() {
  std::string class_name;
  std::cin >> class_name;

  bool has_template_params;
  std::cin >> has_template_params;

  std::optional<std::vector<detail::TemplateParameter>> template_params;
  if (has_template_params) {
    template_params.emplace();
    int num_template_params;
    std::cin >> num_template_params;

    while (num_template_params-- > 0) {
      template_params->emplace_back(read_template_param());
    }
  }

  int num_template_args;
  std::cin >> num_template_args;
  
  std::vector<std::string> template_specialization;
  while(num_template_args-- >0) {
    std::string arg;
    std::cin >> arg;
    template_specialization.push_back(std::move(arg));
  }

  int num_methods, num_variables, num_bases;

  std::cin >> num_methods;
  std::vector<detail::Function> methods;
  methods.reserve(num_methods);
  while (num_methods-- > 0) {
    methods.emplace_back(detail::read_function());
  }

  std::cin >> num_variables;
  std::vector<detail::Var> variables;
  variables.reserve(num_variables);
  while (num_variables-- > 0) {
    variables.emplace_back(detail::read_var());
  }

  std::cin >> num_bases;
  std::vector<detail::Base> bases;
  bases.reserve(num_bases);
  while (num_bases-- > 0) {
    bases.emplace_back(detail::read_base());
  }

  int num_sub_classes;
  std::cin >> num_sub_classes;
  std::vector<detail::Type> sub_types;
  sub_types.reserve(num_sub_classes);
  while (num_sub_classes-- > 0)  {
    sub_types.push_back(read_type());
  }

  return {std::move(class_name), std::move(template_params), std::move(template_specialization), std::move(methods), std::move(variables),
          std::move(bases), std::move(sub_types)};
}
}  // namespace detail

type read_type() {
  return {detail::read_type()};
}
}  // namespace meta

#endif  // META_H
