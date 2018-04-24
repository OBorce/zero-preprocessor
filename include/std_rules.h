#ifndef STD_RULES_H
#define STD_RULES_H

#include <string>

#include <std_ast.h>

#include <boost/spirit/home/x3.hpp>

namespace std_parser::rules {
namespace x3 = boost::spirit::x3;

using x3::alpha;
using x3::char_;
using x3::digit;
using x3::eol;
using x3::lit;

static struct class_type_ : x3::symbols<ast::class_type> {
  class_type_() {
    add("class", ast::class_type::CLASS)("struct", ast::class_type::STRUCT);
  }
} class_type;

x3::rule<class some_space> const some_space = "some_space";
auto const some_space_def = +(eol | ' ' | '\t');

x3::rule<class optionaly_space> const optionaly_space = "optionaly_space";
auto const optionaly_space_def = *(eol | ' ' | '\t');

x3::rule<class include, std::string> const include = "include";
auto const include_def = optionaly_space >> '#' >> *(lit(' ') | '\t') >>
                         lit("include") >> optionaly_space >>
                         (('<' >> *(char_ - '>') >> '>') |
                          ('"' >> *(char_ - '"') >> '"'));

x3::rule<class comment> const comment = "comment";
auto const comment_def =
    optionaly_space >>
    (("//" >> *(char_ - eol)) |
     ("/*" >> x3::repeat[+(char_ - '*') | (lit('*') >> (char_ - '/'))] >>
      "*/"));

x3::rule<class arg_separator> const arg_separator = "arg_separator";
auto const arg_separator_def = optionaly_space >> ',' >> optionaly_space;

x3::rule<class var_operator> const var_operator = "var_operator";
auto const var_operator_def =
    (lit('.') >> -(optionaly_space >> '*')) |
    ('-' >> optionaly_space >> '>' >> -(optionaly_space >> '*'));

x3::rule<class arg_operator> const arg_operator = "arg_operator";
auto const arg_operator_def = optionaly_space >>
                              (lit("++") | "+=" | '+' | "--" | "-=" | '-' |
                               "*=" | '*' | "/=" | '/' | "%=" | '%' | ">>=" |
                               ">>" | ">=" | '>' | "<<=" | "<<" | "<=" | '<' |
                               "&&" | "&=" | '&' | "||" | "|=" | '|' | "~=" |
                               '~' | "^=" | '^' | "!=" | '!' | "==" |
                               '='
                               // TODO: this makes 1.*foo() or a->23 valid
                               | var_operator) >>
                              optionaly_space;

x3::rule<class scope_begin> const scope_begin = "scope_begin";
auto const scope_begin_def = optionaly_space >> '{';

x3::rule<class scope_end> const scope_end = "scope_end";
auto const scope_end_def = optionaly_space >> '}' >> optionaly_space >>
                           -lit(';');

x3::rule<class statement_end> const statement_end = "statement_end";
auto const statement_end_def = optionaly_space >> ';';

x3::rule<class name, std::string> const name = "name";
auto const name_def = ((alpha | char_('_')) >> *(alpha | digit | char_('_')));

// TODO: add nested namespaces c++17
x3::rule<class namespace_begin, std::string> const namespace_begin =
    "namespace_begin";
auto const namespace_begin_def =
    lit("namespace") >> some_space >> name >> scope_begin;

x3::rule<class digits> const digits = "digits";
auto const digits_def = -lit('-') >> optionaly_space >> +digit >>
                        *(lit('\'') >> x3::omit[+digit]);

x3::rule<class integral> const integral = "integral";
auto const integral_def = digits >>
                          -(lit("LLU") | "LLu" | "llU" | "llu" | "LU" | "lU" |
                            "Lu" | "lu" | "LL" | "ll" | 'U' | 'u' | 'L' | 'l');

x3::rule<class floating> const floating = "floating";
auto const floating_def = digits >> '.' >> -digits >>
                          -(lit('f') | 'F' | 'l' | 'L');

x3::rule<class number> const number = "number";
auto const number_def = floating | integral;

// TODO: add support for escaped /" inside string
x3::rule<class string_literal> const string_literal = "string_literal";
auto const string_literal_def = lit('"') >> *(char_ - '"') >> '"';

x3::rule<class char_literal> const char_literal = "char_literal";
auto const char_literal_def = lit('\'') >> (char_ - '\'') >> '\'';

x3::rule<class type_, ast::Type_> const type_ = "type_";
auto const type__def = name >> *(lit("::") >> name);

x3::rule<class var_type, ast::Type> const var_type = "var_type";
x3::rule<class type, ast::Type> const type = "type";
x3::rule<class template_values, ast::TemplateTypes> const template_values =
    "template_values";

auto const var_type_def = (type_) >> -(optionaly_space >> template_values);

auto const template_values_def = '<' >> optionaly_space >>
                                 ((type | digits) % arg_separator) >> '>';

auto const type_def = -(lit("const") >> some_space) >> var_type >>
                      -(-(some_space >> -lit("const")) >> optionaly_space >>
                        ((lit('&') >> -(optionaly_space >> '&')) |
                         +(optionaly_space >> lit('*') >>
                           -(some_space >> lit("const")))));

x3::rule<class argument> const argument = "argument";
x3::rule<class optionaly_arguments> const optionaly_arguments =
    "optionaly_arguments";
x3::rule<class function_call> const function_call = "function_call";
x3::rule<class expression> const expression = "expression";
x3::rule<class paren_expression> const paren_expression = "paren_expression";
x3::rule<class init_list> const init_list = "init_list";
x3::rule<class arg_init_list> const arg_init_list = "arg_init_list";

// TODO: type here denotes a variable name
// change it to variable_type that also covers ::var and templated variables
auto const argument_def = arg_init_list | function_call | var_type | number |
                          char_literal | string_literal | paren_expression;

auto const optionaly_arguments_def =
    -((expression | init_list) % arg_separator);

auto const function_call_def = type >> optionaly_space >> '(' >>
                               optionaly_space >> optionaly_arguments >>
                               optionaly_space >> ')';
auto const expression_def = argument % arg_operator;
auto const paren_expression_def =
    '(' >> optionaly_space >> expression >> optionaly_space >> ')';
auto const init_list_def =
    '{' >> optionaly_space >> optionaly_arguments >> optionaly_space >> '}';

auto const arg_init_list_def = type >> optionaly_space >> init_list;

x3::rule<class statement> const statement = "statement";
auto const statement_def = optionaly_space >> expression >> statement_end;

x3::rule<class return_statement> const return_statement = "return_statement";
auto const return_statement_def = "return" >> some_space >> statement;

// TODO: add optional template after type
x3::rule<class param, ast::var> const param = "param";
auto const param_def = type >> some_space >> name;

x3::rule<class param_optionaly_default, ast::var> const
    param_optionaly_default = "param_optionaly_default";
auto const param_optionaly_default_def = param >> -(optionaly_space >> '=' >>
                                                    optionaly_space >>
                                                    (expression | init_list));

// TODO: support for new / delete ?
x3::rule<class var, ast::var> const var = "var";
auto const var_def = param >> optionaly_space >>
                     -(('=' >> optionaly_space >> expression) |
                       (-('=' >> optionaly_space) >> init_list)) >>
                     optionaly_space >> ';';

x3::rule<class optionaly_params, ast::params> const optionaly_params =
    "optionaly_params";
auto const optionaly_params_def = -(param_optionaly_default % arg_separator);

// TODO: need variadic templates
x3::rule<class template_parameter, std::string> const template_parameter =
    "template_parameter";
auto const template_parameter_def =
    x3::omit[(lit("typename") | "class" | type)] >> some_space >> name >>
    -(optionaly_space >> '=' >> optionaly_space >> x3::omit[(type | number)]);

x3::rule<class template_parameters, std::vector<std::string>> const
    template_parameters = "template_parameters";
auto const template_parameters_def =
    lit("template") >> optionaly_space >> '<' >> optionaly_space >>
    template_parameter % arg_separator >> optionaly_space >> '>';

x3::rule<class function_signiture, ast::function_signiture> const
    function_signiture = "function_signiture";
auto const function_signiture_def =
    -(template_parameters >> optionaly_space) >> type >> some_space >> name >>
    optionaly_space >> '(' >> optionaly_space >> optionaly_params >> ')';

// TODO: name of class can be namespace::name
x3::rule<class class_or_struct, ast::class_or_struct> const class_or_struct =
    "class_or_struct";
auto const class_or_struct_def = -(template_parameters >> optionaly_space) >>
                                 class_type >> some_space >> name;

BOOST_SPIRIT_DEFINE(some_space, optionaly_space, include, comment,
                    arg_separator, arg_operator, var_operator, scope_begin,
                    scope_end, namespace_begin, statement_end, name, type_,
                    type, var_type, template_values, digits, integral, floating,
                    number, string_literal, char_literal, argument,
                    optionaly_arguments, function_call, expression,
                    paren_expression, init_list, arg_init_list,
                    optionaly_params, statement, return_statement, param,
                    param_optionaly_default, var, template_parameter,
                    template_parameters, function_signiture, class_or_struct);
}  // namespace std_parser::rules

#endif  //! STD_RULES_H
