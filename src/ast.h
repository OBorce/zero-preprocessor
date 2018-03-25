#include <boost/fusion/include/adapt_struct.hpp>

namespace parser::ast {
struct var {
  std::vector<std::string> type;
  std::string name;
};

struct function {
  std::vector<std::string> return_type;
  std::string name;
  std::vector<var> parameters;
};

enum class class_type { CLASS, STRUCT };

struct class_or_struct {
  class_type type;
  std::string name;
};
}  // namespace parser::ast

BOOST_FUSION_ADAPT_STRUCT(parser::ast::var, type, name)
BOOST_FUSION_ADAPT_STRUCT(parser::ast::function, return_type, name, parameters)
BOOST_FUSION_ADAPT_STRUCT(parser::ast::class_or_struct, type, name)
