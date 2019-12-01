#ifndef GEN_UTILS_H
#define GEN_UTILS_H

#include <algorithm>
#include <iostream>
#include <iterator>

#include <std_ast.hpp>
#include <std_helpers.hpp>

#include <meta_classes_rules.hpp>
#include <meta_process.hpp>

namespace helper = std_parser::rules::ast;
namespace meta_classes {
// TODO: replace with a range when we have them
template <class Iter>
auto gen_target_output(rules::ast::Target& t, Iter begin, Iter end) {
  std::string out;
  out.reserve(100);
  out += t.target;

  auto& outputs = t.output;
  outputs.erase(std::remove_if(outputs.begin(), outputs.end(),
                               [](auto& s) { return s.empty(); }),
                outputs.end());

  // move begin to the closing bracket of ->(target)
  begin = std::find(begin, end, ')');

  if (auto start_bracket = std::find(begin, end, '{'); start_bracket != end) {
    begin = start_bracket + 1;
    for (auto& s : outputs) {
      auto it = std::search(begin, end, s.begin(), s.end());
      out += " << \"";
      // TODO: maybe we need to escape " characters or use raw string
      out.append(begin, it);

      out += "\" << ";
      out += s;
      begin = std::find(it + s.size(), end, '$') + 1;
    }

    auto rbeg = std::make_reverse_iterator(begin);
    auto rend = std::make_reverse_iterator(end);
    auto it = std::find(rend, rbeg, '}');
    auto dist = std::distance(rend, it) + 1;
    out += " << \"";
    // TODO: maybe we need to escape " characters or use raw string
    out.append(begin, end - dist);
    out += "\";";
  } else {
    out += " << ";
    out.append(begin + 1, end);
  }

  return out;
}

template <class Container>
std::string gen_main(Container& meta_classes) {
  std::string out;
  out.reserve(100);
  out +=
      "static const std::unordered_map<std::string, void(*)(meta::type,const "
      "meta::type)> funs {\n";
  for (auto& meta_class : meta_classes) {
    out += "{\"";
    out += meta_class;
    out += "\", &";
    out += meta_class;
    out += "},";
  }

  out.pop_back();
  out += "};";

  out += R"main(
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
int main(int argc, char* argv[]) {
#ifdef _WIN32
  _setmode( _fileno( stdout ),  _O_BINARY );
#endif
  int mode;
  while (true) {
    std::cin >> mode;
    switch (mode) {
      case 1: {
        std::cout << funs.size() << std::endl;
        for (auto& kv : funs) {
          std::cout << kv.first << '\n';
        }
        break;
      }
      case 2: {
        std::string fun;
        std::cin >> fun;
        std::string output;
        {
          auto const type = meta::read_type();
          meta::type t{type.name()};
          auto f = funs.at(fun);
          f(t, type);
          output = t.get_representation();
        }
        std::cout << 0 << '\n';
        std::cout << output.size() << '\n';
        std::cout << output << std::endl;
        break;
      }
      case 3:
        return 0;
      default:
        std::cout << "unknown mode " << mode;
    }
  }

  return 0;
}
)main";
  return out;
}

template <typename Writer>
void write_location(std_parser::rules::ast::SourceLocation const& loc,
                    Writer& writer) {
  writer << loc.row << '\n';
  writer << loc.col << '\n';
}

template <typename Writer>
void write_type(std_parser::rules::ast::Type const& type, Writer& writer) {
  helper::serialize(writer, type);
}

using AccessModifier = std_parser::rules::ast::access_modifier;
template <typename Writer>
void write_function(std_parser::rules::ast::Function const& fun,
                    AccessModifier modifier, Writer& writer) {
  write_location(fun.loc, writer);
  write_type(fun.return_type, writer);

  writer << fun.is_virtual << '\n';

  writer << static_cast<int>(fun.constructor_type) << '\n';

  writer << static_cast<int>(modifier) << '\n';

  writer << fun.name << '\n';

  auto& params = fun.parameters.parameters;
  writer << params.size() << '\n';

  for (auto& p : params) {
    write_type(p.type, writer);
    writer << p.name << '\n';
  }

  writer << fun.is_const << '\n';

  writer << static_cast<int>(fun.qualifier) << '\n';

  writer << fun.is_noexcept << '\n';

  writer << fun.is_override << '\n';

  writer << fun.is_pure_virtual << '\n';

  writer << fun.body.size() << '\n';

  if (!fun.body.empty()) {
    writer << fun.body << '\n';
  }
}

template <typename Writer>
void write_methods(std::vector<std_parser::rules::ast::Function> const& methods,
                   AccessModifier modifier, Writer& writer) {
  for (auto& m : methods) {
    write_function(m, modifier, writer);
  }
}
template <typename Writer>
void write_operator(std_parser::rules::ast::Operator const& op,
                    Writer& writer) {
  using OP = std_parser::rules::ast::Operator;
  switch (op) {
    case OP::PlusEq:
      writer << "+=";
      break;
    case OP::Plus:
      writer << "+";
      break;
    case OP::ArrowDeref:
      writer << "->*";
      break;
    case OP::Arrow:
      writer << "->";
      break;
    case OP::MinusEq:
      writer << "-=";
      break;
    case OP::Minus:
      writer << "-";
      break;
    case OP::DotDeref:
      writer << ".*";
      break;
    case OP::Dot:
      writer << ".";
      break;
    case OP::MultiplyEq:
      writer << "*=";
      break;
    case OP::Multiply:
      writer << "*";
      break;
    case OP::DivideEq:
      writer << "/=";
      break;
    case OP::Divide:
      writer << "/";
      break;
    case OP::ModuleEq:
      writer << "%=";
      break;
    case OP::Module:
      writer << "%";
      break;
    case OP::RShiftEq:
      writer << ">>=";
      break;
    case OP::RShift:
      writer << ">>";
      break;
    case OP::GtEq:
      writer << ">=";
      break;
    case OP::Gt:
      writer << ">";
      break;
    case OP::LShiftEq:
      writer << "<<=";
      break;
    case OP::LShift:
      writer << "<<";
      break;
    case OP::LtEq:
      writer << "<=";
      break;
    case OP::Lt:
      writer << "<";
      break;
    case OP::And:
      writer << "&&";
      break;
    case OP::BitAndEq:
      writer << "&=";
      break;
    case OP::BitAnd:
      writer << "&";
      break;
    case OP::Or:
      writer << "||";
      break;
    case OP::BitOrEq:
      writer << "|=";
      break;
    case OP::BitOr:
      writer << "|";
      break;
    case OP::TildeEq:
      writer << "~=";
      break;
    case OP::Tilde:
      writer << "~";
      break;
    case OP::BitXorEq:
      writer << "^=";
      break;
    case OP::BitXor:
      writer << "^";
      break;
    case OP::NotEq:
      writer << "!=";
      break;
    case OP::Not:
      writer << "!";
      break;
    case OP::EqEq:
      writer << "==";
      break;
    case OP::Eq:
      writer << "=";
      break;
    case OP::Comma:
      writer << ",";
      break;
  }
  writer << std::endl;
}

template <typename Writer>
void write_expression_var(std_parser::rules::ast::ExpressionVariant const& exp,
                          Writer& writer) {
  std::visit(
      overloaded{
          [&](std_parser::rules::ast::VariableExpression const& e) {
            writer << helper::to_string(e.expression);
          },
          // TODO: write other expressions
          [](auto&) {},
      },
      exp);
  writer << std::endl;
}

template <typename Writer>
void write_expression(std_parser::rules::ast::Expression const& exp,
                   Writer& writer) {
  auto& exps = exp.expressions;
  auto& ops = exp.operators;
  auto size = exp.expressions.size();
  writer << (size + exp.operators.size()) << std::endl;
  if (size > 0) {
    write_expression_var(exps.front(), writer);
  }
  for (std::size_t i = 1; i < size; i++) {
    write_operator(ops[i - 1], writer);
    write_expression_var(exps[i], writer);
  }
}


template <typename Writer>
void write_variables(std::vector<std_parser::rules::ast::var> const& variables,
                     AccessModifier modifier, Writer& writer) {
  for (auto& v : variables) {
    write_location(v.loc, writer);
    write_type(v.type, writer);
    writer << static_cast<int>(modifier);
    writer << v.name << std::endl;
    write_expression(v.init, writer);
  }
}

template <typename Writer>
void write_bases(
    std::vector<std_parser::rules::ast::UnqulifiedType> const& bases,
    AccessModifier modifier, Writer& writer) {
  for (auto& b : bases) {
    writer << helper::to_string(b) << std::endl;
    writer << static_cast<int>(modifier) << std::endl;
  }
}

template <typename Writer>
void write_template_params(
    std::optional<std_parser::rules::ast::TemplateParameters> const&
        template_params,
    Writer& writer) {
  writer << template_params.has_value() << std::endl;
  if(not template_params) {
    return;
  }

  auto& params = template_params.value();

  writer << params.size() << std::endl;
  for (auto& p : params) {
    writer << helper::join(p.type, "::") << std::endl;
    writer << p.name << std::endl;
  }
}

template <typename Writer>
void write_template_specialization(
    std_parser::rules::ast::TemplateTypes const& template_specialization,
    Writer& writer) {
  auto& args = template_specialization.template_types;
  writer << args.size() << std::endl;
  for (auto& p : args) {
    std::visit(overloaded{
                   [&](std_parser::rules::ast::LiteralExpression const& l) {
                     writer << helper::to_string(l.lit);
                   },
                   [&](std_parser::rules::ast::Type const& t) {
                     writer << helper::to_string(t);
                   },
               },
               p);
    writer << std::endl;
  }
}

template <typename Writer>
void write_class(std_parser::rules::ast::Class& cls, Writer& writer) {
  writer << cls.name << std::endl;

  write_template_params(cls.template_parameters, writer);

  write_template_specialization(cls.specialization, writer);

  writer << (cls.public_methods.size() + cls.private_methods.size() +
             cls.protected_methods.size() + cls.unspecified_methods.size())
         << std::endl;

  write_methods(cls.public_methods, AccessModifier::PUBLIC, writer);

  write_methods(cls.private_methods, AccessModifier::PROTECTED, writer);

  write_methods(cls.protected_methods, AccessModifier::PRIVATE, writer);

  write_methods(cls.unspecified_methods, AccessModifier::UNSPECIFIED, writer);

  writer << (cls.public_members.size() + cls.private_members.size() +
             cls.protected_members.size() + cls.unspecified_members.size())
         << std::endl;

  write_variables(cls.public_members, AccessModifier::PUBLIC, writer);
  write_variables(cls.private_members, AccessModifier::PROTECTED, writer);
  write_variables(cls.protected_members, AccessModifier::PRIVATE, writer);
  write_variables(cls.unspecified_members, AccessModifier::UNSPECIFIED, writer);

  writer << (cls.public_bases.size() + cls.private_bases.size() +
             cls.protected_bases.size())
         << std::endl;

  write_bases(cls.public_bases, AccessModifier::PUBLIC, writer);
  write_bases(cls.private_bases, AccessModifier::PROTECTED, writer);
  write_bases(cls.protected_bases, AccessModifier::PRIVATE, writer);


  std::size_t num_sub_classes = 0;
  for(auto it : cls.classes) {
    num_sub_classes += it.second.size();
  }
  writer << num_sub_classes << std::endl;
  for(auto it : cls.classes) {
    for(auto& c : it.second){
      write_class(c, writer);
    }
  }
}

enum class ParsedResult { OK, Error };

template <class StdParser, class ErrorReporter>
void handle_meta_process_request(MetaProcess& process, StdParser& std_parser,
                                 std::string_view request,
                                 ErrorReporter& reporter) {
  std::cout << "Recieved request: " << request;
  auto out = std_parser.try_parse_entire_class(request.begin(), request.end());
  if (out.result) {
    process.output << static_cast<int>(ParsedResult::OK) << std::endl;
    write_class(*out.result, process.output);
  } else {
    process.output << static_cast<int>(ParsedResult::Error) << std::endl;
    std::string msg = "can't parse: \n";
    std::size_t remaining = std::distance(out.processed_to, request.end());
    std::size_t size = std::min<std::size_t>(30ul, remaining);
    msg += std::string_view{&*out.processed_to, size};
    reporter(msg);

    std::exit(EXIT_FAILURE);
  }
}

template <typename StdParser, class ErrorReporter>
std::string gen_meta_class(MetaProcess& process,
                           const std::string_view meta_class,
                           std_parser::rules::ast::Class& cls,
                           StdParser& std_parser, ErrorReporter& reporter) {
  std::cout << "getting output from meta process for metaclass " << meta_class
            << "\n";
  // TODO: add an enum for meta process request type
  process.output << 2 << std::endl;
  process.output << meta_class << std::endl;

  write_class(cls, process.output);

  std::string output, line;
  int status;
  std::size_t output_size;

  do {
    output.clear();
    process.input >> status;
    process.input >> output_size;
    process.input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    output.resize(output_size);
    process.input.read(output.data(), output_size);

    switch (status) {
      case -1: {
        reporter(output);
        // TODO: throw exception and catch in main to guarantee resource cleanup
        std::exit(EXIT_FAILURE);
      }
      case 1: {
        handle_meta_process_request(process, std_parser, output, reporter);
      }
    }
  } while (status != 0);

  return output;
}
}  // namespace meta_classes

#endif  // GEN_UTILS_H
