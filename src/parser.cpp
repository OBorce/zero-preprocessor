#include <boost/spirit/home/x3.hpp>
#include <iostream>
#include <string>
#include <tuple>

#include <ast>

namespace parser {
namespace x3 = boost::spirit::x3;

using boost::spirit::x3::alpha;
using boost::spirit::x3::char_;
using boost::spirit::x3::digit;
using boost::spirit::x3::eol;
using boost::spirit::x3::lit;

struct class_type_ : x3::symbols<ast::class_type> {
  class_type_() {
    add("class", ast::class_type::CLASS)("struct", ast::class_type::STRUCT);
  }
} class_type;

x3::rule<class some_space> const some_space = "some_space";
auto const some_space_def = +(eol | ' ' | '\t');

x3::rule<class optionaly_space> const optionaly_space = "optionaly_space";
auto const optionaly_space_def = *(eol | ' ' | '\t');

x3::rule<class name, std::string> const name = "name";
auto const name_def = ((alpha | char_('_')) >> *(alpha | digit | char_('_')));

x3::rule<class type, std::vector<std::string>> const type = "type";
auto const type_def = name >> *(lit("::") >> name);

// TODO: add optional template after type
x3::rule<class param, ast::var> const param = "param";
auto const param_def = type >> some_space >> name;

// TODO: optional initialization to variable
x3::rule<class var, ast::var> const var = "var";
auto const var_def = param >> optionaly_space >> ';';

x3::rule<class optionaly_params, std::vector<ast::var>> const optionaly_params =
    "optionaly_params";
auto const optionaly_params_def =
    -(param % (optionaly_space >> ',' >> optionaly_space));

// TODO: add optianly templates before return type
x3::rule<class function, ast::function> const function = "function";
auto const function_def = type >> some_space >> name >> optionaly_space >>
                          '(' >> optionaly_space >> optionaly_params >> ')';

// TODO: add optianly templates before class or struct
x3::rule<class class_or_struct, ast::class_or_struct> const class_or_struct =
    "class_or_struct";
auto const class_or_struct_def = class_type >> some_space >> name;

BOOST_SPIRIT_DEFINE(some_space, optionaly_space, name, type, param,
                    optionaly_params, var, function, class_or_struct);
}  // namespace parser

template <typename Iterator>
bool parse_name(Iterator& begin, Iterator end) {
  namespace x3 = boost::spirit::x3;

  parser::ast::var var;
  bool r = x3::parse(begin, end, parser::var, var);
  std::cout << "found: ";
  for (auto& s : var.type) std::cout << s << " ";
  std::cout << "Name " << var.name << std::endl;
  return r;
}

template <typename Iterator>
bool parse_class(Iterator& begin, Iterator end) {
  namespace x3 = boost::spirit::x3;

  parser::ast::class_or_struct class_or_struct;
  bool r = x3::parse(begin, end, parser::class_or_struct, class_or_struct);
  std::cout << "found: ";
  std::cout << "Class Name: " << class_or_struct.name;
  std::cout << " Class type: " << static_cast<int>(class_or_struct.type)
            << std::endl;
  return r;
}

template <typename Iterator>
bool parse_function(Iterator& begin, Iterator end) {
  namespace x3 = boost::spirit::x3;

  parser::ast::function fun;
  bool r = x3::parse(begin, end, parser::function, fun);
  std::cout << "found: ";
  std::cout << "Function Name: " << fun.name << '\n';
  std::cout << "Function Return type: ";
  for (auto& t : fun.return_type) std::cout << t << " ";
  std::cout << "\nFunction input parameters: ";
  for (auto& p : fun.parameters) {
    for (auto& t : p.type) std::cout << t << " ";

    std::cout << p.name << "; ";
  }

  std::cout << '\n';

  return r;
}

int main(int argn, char** argv) {
  std::string s = argv[1];
  std::cout << "parsing " << s << std::endl;
  auto beg = s.begin();
  // std::cout << parse_name(beg, s.end());
  // std::cout << parse_class(beg, s.end());
  std::cout << parse_function(beg, s.end());
  bool isEnd = beg == s.end();
  bool isBegin = beg == s.begin();
  std::cout << isEnd << " " << isBegin << std::endl;
  return 0;
}
