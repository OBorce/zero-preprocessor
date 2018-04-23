#include <array>
#include <iostream>
#include <string>

#include <std_rules.h>

#include <catch.hpp>

using std::string_literals::operator""s;
using namespace std_parser;

template <typename Rule>
class CanParse : public Catch::MatcherBase<std::string> {
  Rule rule;
  std::string rule_name;

 public:
  CanParse(Rule r, std::string_view rule_name)
      : rule(r), rule_name{rule_name} {}

  // Performs the test for this matcher
  virtual bool match(std::string const& str) const override {
    namespace x3 = boost::spirit::x3;

    auto begin = str.begin();
    auto end = str.end();
    auto w = [this](auto& ctx) { std::cout << rule_name << " " << std::endl; };

    bool r = x3::parse(begin, end, rule[w]);
    return r && begin == end;
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "Trying to be parsed as a/an " << rule_name;
    return ss.str();
  }
};

TEST_CASE("Parse valid number", "[number]") {
  std::array valid_numbers{"1"s,
                           "123"s,
                           "-123"s,
                           "123l"s,
                           "3213L"s,
                           "3213213LL"s,
                           "123'1231'321ll"s,
                           "123.213"s,
                           "312."s,
                           "12312.f"s,
                           "3.1f"s,
                           "-3.1f"s,
                           "321'321.001f"s,
                           "123.21F"s,
                           "123.131'123L"s,
                           "12312.13L"s};

  for (auto& valid_number : valid_numbers) {
    REQUIRE_THAT(valid_number, CanParse(rules::number, "number"));
  }
}

TEST_CASE("Parse valid type", "[type]") {
  // TODO add types with templates when we support them
  std::array valid_types{"int"s,
                         "std::string"s,
                         "rules::ast::val"s,
                         "const int"s,
                         "auto&"s,
                         "auto&&"s,
                         "const auto&&"s,
                         "const auto*"s,
                         "const auto **"s,
                         "const auto * *"s,
                         "const auto** const"s,
                         "auto* const * const"s,
                         "some_Type"s};

  for (auto& valid_type : valid_types) {
    REQUIRE_THAT(valid_type, CanParse(rules::type, "type"));
  }
}

TEST_CASE("Parse valid expression", "[expression]") {
  std::array valid_expressions{"some_variable"s,
                               "a + b"s,
                               "(a)"s,
                               "'1'"s,
                               "(a + (b))"s,
                               "(((a - b)) % c) * d"s,
                               "d - (a) / ((b))"s,
                               "std::min(a, b)"s,
                               "foo(a + b, baz(c) * 2)"s,
                               "(a * foo((b))) -c"s,
                               "std::span{beg, end}"s,
                               "std::string(\"hello\")"s,
                               "baz(\"hello\", 3 + 2)"s,
                               "a - b"s,
                               "a * b"s};

  for (auto& valid_expression : valid_expressions) {
    REQUIRE_THAT(valid_expression, CanParse(rules::expression, "expression"));
  }
}

TEST_CASE("Parse valid variables", "[var]") {
  std::array valid_vars{"int a;"s,
                        "std::string s{\"hello\"};"s,
                        "std::vector<int> v;"s,
                        "std::vector<int> v {};"s,
                        "std::vector<int> v {1, 2, 3};"s,
                        "std::pair<int, float> v {1, 2.0f};"s,
                        "std::pair<int, std::vector<char>> v {1, {}};"s,
                        "std::array<int, 4> a;"s,
                        "rules::ast::val v = 2;"s,
                        "rules::ast::val v = {2, foo(a)};"s,
                        "nsd::asd::varr v23 {2, 3, baz(1, 3)};"s,
                        "some_Type var_a2 = foo(2) ;"s};

  for (auto& valid_var : valid_vars) {
    REQUIRE_THAT(valid_var, CanParse(rules::var, "variable"));
  }
}

TEST_CASE("Parse valid statements", "[statements]") {
  std::array valid_statements{"a += 1;"s,
                              "a = 2 + 3;"s,
                              "a -= std::min(2, b);"s,
                              "std::foo(a + b, c);"s,
                              "std::cout << 2;"s,
                              "std::cout << asd << 2 << std::endl;"s};

  for (auto& valid_statement : valid_statements) {
    REQUIRE_THAT(valid_statement, CanParse(rules::statement, "statement"));
  }
}

TEST_CASE("Parse valid function_signitures", "[function_signiture]") {
  std::array valid_function_signitures{
      "int a()"s,
      "std::string s(int i)"s,
      "rules::ast::val some_name(std::string s, int a)"s,
      "auto def_arg(int a = 2, long l = 3l)"s,
      "template<typename T> auto def_arg(T a = 2, long l = 3l)"s,
      "template<typename T, int N = 0> auto def_arg(T a = 2, long l = 3l)"s,
      "some_Type function_a2(std::asd::ddd d)"s};

  for (auto& valid_function_signiture : valid_function_signitures) {
    REQUIRE_THAT(valid_function_signiture,
                 CanParse(rules::function_signiture, "function_signiture"));
  }
}

TEST_CASE("Parse valid class", "[class]") {
  std::array valid_classes{
      "class A"s,
      "struct point"s,
      "template <typename T> class Test"s,
      "template <typename T, class B> class Test"s,
      "template <int N, class B> class Test"s,
      "template <std::size_t N, class B> class Test"s,
      "template <std::size_t N = 2, class B = int> class Test"s,
      "template <std::size_t N = 2, class B = std::size_t> class Test"s,
  };

  for (auto& valid_class : valid_classes) {
    REQUIRE_THAT(valid_class,
                 CanParse(rules::class_or_struct, "class or struct"));
  }
}
