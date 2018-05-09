#ifndef META_CLASSES_RULES_H
#define META_CLASSES_RULES_H
#include <boost/spirit/home/x3.hpp>

namespace meta_classes::rules {
namespace x3 = boost::spirit::x3;

using x3::eol, x3::alpha, x3::char_, x3::digit, x3::lit;

x3::rule<class optionaly_space> const optionaly_space = "optionaly_space";
auto const optionaly_space_def = *(eol | ' ' | '\t');

x3::rule<class const_expression> const const_expression = "const_expression";
auto const const_expression_def = optionaly_space >> "constexpr";

x3::rule<class name, std::string> const name = "name";
auto const name_def = ((alpha | char_('_')) >> *(alpha | digit | char_('_')));

x3::rule<class target, std::string> const target = "target";
auto const target_def = optionaly_space >> "->(" >> name >> ")";

x3::rule<class meta_target, std::string> const meta_target = "meta_target";
auto const meta_target_def = name >> char_('.') >> name >> "()$";

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
auto const target_out_def = optionaly_space >> '{' >> meta_target_out >> "};";

BOOST_SPIRIT_DEFINE(optionaly_space, const_expression, name, target,
                    meta_target, meta_target_out, meta_target_out_braced,
                    meta_target_out_no_braced, target_out);
}  // namespace meta_classes::rules

#endif  //! META_CLASSES_RULES_H
