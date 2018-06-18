#ifndef META_CLASSES_RULES_H
#define META_CLASSES_RULES_H
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>

namespace meta_classes::rules {
namespace x3 = boost::spirit::x3;

namespace ast {
struct Target {
  std::string target;
  std::vector<std::string> output;
};

struct MetaClass {
  std::string meta_class_name;
  std::string name;
};
}  // namespace ast

using x3::eol, x3::alpha, x3::char_, x3::digit, x3::lit;

x3::rule<class optionaly_space> const optionaly_space = "optionaly_space";
auto const optionaly_space_def = *(eol | ' ' | '\t');

x3::rule<class scope_end> const scope_end = "scope_end";
auto const scope_end_def = optionaly_space >> '}' >> optionaly_space >>
                           lit(';');

x3::rule<class const_expression> const const_expression = "const_expression";
auto const const_expression_def = optionaly_space >> "constexpr";

x3::rule<class name, std::string> const name = "name";
auto const name_def = ((alpha | char_('_')) >> *(alpha | digit | char_('_')));

x3::rule<class selected_target, std::string> const selected_target =
    "selected_target";
auto const selected_target_def = optionaly_space >> "->(" >> name >> ")";

x3::rule<class meta_expression, std::string> const meta_expression =
    "meta_expression";
auto const meta_expression_def = name >> *(char_('.') >> name) >>
                                 -(char_('(') >> optionaly_space >> char_(')'));

x3::rule<class meta_target, std::string> const meta_target = "meta_target";
auto const meta_target_def =
    ('(' >> meta_expression >> ')' >> '$') | (meta_expression >> '$');

x3::rule<class meta_target_out, std::vector<std::string>> const
    meta_target_out = "meta_target_out";
x3::rule<class meta_target_out_no_braced, std::vector<std::string>> const
    meta_target_out_no_braced = "meta_target_out_no_braced";
x3::rule<class meta_target_out_braced, std::vector<std::string>> const
    meta_target_out_braced = "meta_target_out_braced";

auto const meta_target_out_no_braced_def =
    +(meta_target | x3::omit[char_ - (lit('{') | '}')]);

auto const meta_target_out_braced_def = '{' >> meta_target_out >> '}';

auto const meta_target_out_def =
    *(meta_target_out_no_braced | meta_target_out_braced);

x3::rule<class target_out, std::vector<std::string>> const target_out =
    "target_out";
auto const target_out_def =
    (name >> optionaly_space >> ';') | ('{' >> meta_target_out >> "};");

x3::rule<class target, ast::Target> const target = "target";
auto const target_def = selected_target >> optionaly_space >> target_out;

BOOST_SPIRIT_DEFINE(optionaly_space, scope_end, const_expression, name,
                    selected_target, meta_expression, meta_target,
                    meta_target_out, meta_target_out_braced,
                    meta_target_out_no_braced, target_out, target);
}  // namespace meta_classes::rules

BOOST_FUSION_ADAPT_STRUCT(meta_classes::rules::ast::Target, target, output)
BOOST_FUSION_ADAPT_STRUCT(meta_classes::rules::ast::MetaClass, meta_class_name,
                          name)

#endif  //! META_CLASSES_RULES_H
