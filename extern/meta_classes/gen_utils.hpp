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
int main(int argc, char* argv[]) {
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
        auto type = meta::read_type();
        auto const t2 = type;
        auto f = funs.at(fun);
        f(type, t2);
        std::string output = type.get_representation();
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
void write_type(std_parser::rules::ast::Type const& type, Writer& writer) {
  helper::serialize(writer, type);
}

using AccessModifier = std_parser::rules::ast::access_modifier;
template <typename Writer>
void write_methods(std::vector<std_parser::rules::ast::Function> const& methods,
                   AccessModifier modifier, Writer& writer) {
  for (auto& m : methods) {
    write_type(m.return_type, writer);

    writer << m.is_virtual << '\n';

    writer << static_cast<int>(m.constructor_type) << '\n';

    writer << static_cast<int>(modifier) << '\n';

    writer << m.name << '\n';

    auto& params = m.parameters.parameters;
    writer << params.size() << '\n';

    for (auto& p : params) {
      write_type(p.type, writer);
      writer << p.name << '\n';
    }

    writer << m.is_const << '\n';

    writer << static_cast<int>(m.qualifier) << '\n';

    writer << m.is_override << '\n';
  }
}

template <typename Writer>
void write_variables(std::vector<std_parser::rules::ast::var> const& variables,
                     AccessModifier modifier, Writer& writer) {
  for (auto& v : variables) {
    write_type(v.type, writer);
    writer << static_cast<int>(modifier);
    writer << v.name << std::endl;
  }
}

std::string gen_meta_class(MetaProcess& process,
                           const std::string_view meta_class,
                           std_parser::rules::ast::Class& cls) {
  std::cout << "getting output from meta process\n";
  process.output << 2 << std::endl;
  process.output << meta_class << std::endl;

  process.output << cls.name << std::endl;
  process.output << (cls.public_methods.size() + cls.private_methods.size() +
                     cls.protected_methods.size() +
                     cls.unspecified_methods.size())
                 << std::endl;

  write_methods(cls.public_methods, AccessModifier::PUBLIC, process.output);

  write_methods(cls.private_methods, AccessModifier::PROTECTED, process.output);

  write_methods(cls.protected_methods, AccessModifier::PRIVATE, process.output);

  write_methods(cls.unspecified_methods, AccessModifier::UNSPECIFIED,
                process.output);

  process.output << (cls.public_members.size() + cls.private_members.size() +
                     cls.protected_members.size() +
                     cls.unspecified_members.size())
                 << std::endl;

  write_variables(cls.public_members, AccessModifier::PUBLIC, process.output);
  write_variables(cls.private_members, AccessModifier::PROTECTED,
                  process.output);
  write_variables(cls.protected_members, AccessModifier::PRIVATE,
                  process.output);
  write_variables(cls.unspecified_members, AccessModifier::UNSPECIFIED,
                  process.output);

  std::string output, line;
  int status;
  std::size_t output_size;
  process.input >> status;
  process.input >> output_size;
  output.reserve(output_size);
  while (output.size() < output_size) {
    std::getline(process.input, line);
    output += line;
    output += '\n';
  }

  if (status == 0) {
    return output;
  }

  std::cerr << output << std::endl;
  std::terminate();
}
}  // namespace meta_classes

#endif  // GEN_UTILS_H
