#include <array>
#include <catch2/catch.hpp>
#include <iostream>
#include <std_rules.hpp>
#include <string>

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

TEST_CASE("Parse valid integral", "[number]") {
  std::array valid_numbers{
      "1"s, "123"s, "-123"s, "123l"s, "3213L"s, "3213213LL"s, "123'1231'321ll"s,
  };

  for (auto& valid_number : valid_numbers) {
    REQUIRE_THAT(valid_number, CanParse(rules::integral, "integral"));
  }
}

TEST_CASE("Parse valid floating", "[number]") {
  std::array valid_numbers{
      "12312.f"s, "3.1f"s, "-3.1f"s, "321'321.001f"s, "123.21F"s,
  };

  for (auto& valid_number : valid_numbers) {
    REQUIRE_THAT(valid_number, CanParse(rules::floating, "floating"));
  }
}

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

TEST_CASE("Parse valid quoted_string", "[quoted_string]") {
  std::array valid_quoted_strings{"\"asd\""s, "\"asd\" \"asd \""s};

  for (auto& valid_quoted_string : valid_quoted_strings) {
    REQUIRE_THAT(valid_quoted_string,
                 CanParse(rules::quoted_strings, "quoted_string"));
  }
}

TEST_CASE("Parse valid type", "[type]") {
  std::array valid_types{"int"s,
                         "std::string"s,
                         "rules::ast::val"s,
                         "std::vector<int>"s,
                         "std::array<int, 5>"s,
                         "usr::Foo<int>::Type"s,
                         "usr::Name<int, \"foo\">"s,
                         "const int"s,
                         "auto&"s,
                         "auto&&"s,
                         "const auto&&"s,
                         "const auto*"s,
                         "const auto **"s,
                         "const auto * *"s,
                         "const std::vector<std::vector<char>>"s,
                         "const auto** const"s,
                         "auto* const * const"s,
                         "const std::vector<const std::vector<char>>"s,
                         "some_Type"s};

  for (auto& valid_type : valid_types) {
    REQUIRE_THAT(valid_type, CanParse(rules::type, "type"));
  }
}

TEST_CASE("Parse valid expression old", "[expression]") {
  std::array valid_expressions{"some_variable"s,
                               "a + b"s,
                               "(a)"s,
                               "i++"s,
                               "i < 2"s,
                               "'1'"s,
                               "(a + (b))"s,
                               "!foo(a + (b))"s,
                               "(a && !(b || !c))"s,
                               "(((a - b)) % c) * d"s,
                               "d - (a) / ((b))"s,
                               "std::min(a, b)"s,
                               "foo(a + b, baz(c) * 2)"s,
                               "(a * foo((b))) -c"s,
                               "std::span{beg, end}"s,
                               "std::string(\"hello\")"s,
                               "baz(\"hello\", 3 + 2)"s,
                               "a - b"s,
                               "a && !b"s,
                               "a.foo()"s,
                               "a.foo(i, std::string{})"s,
                               "a * b"s};

  for (auto& valid_expression : valid_expressions) {
    REQUIRE_THAT(valid_expression,
                 CanParse(rules::expression_old, "expression"));
  }
}

TEST_CASE("Parse valid variables old", "[var]") {
  std::array valid_vars{"int a"s,
                        "std::string s{\"hello\"}"s,
                        "std::vector<int> v"s,
                        "int i = 0"s,
                        "a && b"s,
                        "std::vector<int> v {}"s,
                        "std::vector<int> v {1, 2, 3}"s,
                        "std::pair<int, float> v {1, 2.0f}"s,
                        "std::pair<int, std::vector<char>> v {1, {}}"s,
                        "std::array<int, 4> a"s,
                        "rules::ast::val v = 2"s,
                        "rules::ast::val v = {2, foo(a)}"s,
                        "nsd::asd::varr v23 {2, 3, baz(1, 3)}"s,
                        "some_Type var_a2 = foo(2) "s};

  for (auto& valid_var : valid_vars) {
    REQUIRE_THAT(valid_var, CanParse(rules::var_old, "variable"));
  }
}

TEST_CASE("Parse valid for loop", "[for_loop]") {
  std::array valid_for_loops{"for (;;)"s,
                             "for(int i = 0;;)"s,
                             "for(int i = 0; i < 2;)"s,
                             "for (int i = 0; i < 2; i++)"s,
                             "for (int i = 0; i < 2;)"s,
                             "for (; i < 2; i++)"s,
                             "for (auto& e : elems)"s};

  for (auto& valid_for_loop : valid_for_loops) {
    REQUIRE_THAT(valid_for_loop, CanParse(rules::for_loop, "for_loop"));
  }
}

TEST_CASE("Parse valid while loop", "[while_loop]") {
  std::array valid_while_loops{"while (true)"s,
                               "while(a && b)"s,
                               "while(i < 10)"s,
                               "while (std::getline(s, in))"s,
                               "while (int i = get_n())"s,
                               "while (k || p)"s,
                               "while (k || p && s)"s,
                               "while (k && p || s)"s,
                               "while (k && p || s && t)"s};

  for (auto& valid_while_loop : valid_while_loops) {
    REQUIRE_THAT(valid_while_loop, CanParse(rules::while_loop, "while_loop"));
  }
}

TEST_CASE("Parse valid for lambda capture", "[lambda]") {
  std::array valid_for_loops{
      "[]"s,         "[&]"s,    "[=]"s,       "[=, &a]"s,
      "[&, &a]"s,    "[=, a]"s, "[=, a, b]"s, "[=, this, b, &a]"s,
      "[&c, b, &d]"s};

  for (auto& valid_for_loop : valid_for_loops) {
    REQUIRE_THAT(valid_for_loop,
                 CanParse(rules::lambda_capture, "lambda_capture"));
  }
}

TEST_CASE("Parse valid function_signatures", "[function_signature]") {
  std::array valid_function_signatures{
      "int a()"s,
      "std::string s(int i)"s,
      "void s(const int i)"s,
      "constexpr int s(const int i)"s,
      "void s(const int i) noexcept"s,
      "std::string no_name_params(int, float f, double)"s,
      "std::string no_name_params2(int, int , float f, double)"s,
      "rules::ast::val some_name(std::string s, int a)"s,
      "auto& def_arg(int a = 2, long l = 3l)"s,
      "template<typename T> auto def_arg(T a = 2, long l = 3l)"s,
      "template<typename T> auto def_arg(T a = 2, long l = 3l) noexcept(sizeof(T) > 2)"s,
      "template<typename T, int N = 0> auto def_arg(T a = 2, long l = 3l)"s,
      "const some_Type* function_a2(std::asd::ddd d)"s};

  for (auto& valid_function_signature : valid_function_signatures) {
    REQUIRE_THAT(valid_function_signature,
                 CanParse(rules::function_signature_old, "function_signature"));
  }
}

TEST_CASE("Parse valid user class deduction guide",
          "[deduction_guide]") {
  std::array valid_deduction_guides{
      "template<typename T> Foo(T a, long l) -> Foo<T>;"s,
      "template<typename T> Foo(T&) -> Foo<T>;"s,
      "template<typename T> Foo(const T&) -> Foo<T&>;"s,
      };

  for (auto& valid_deduction_guide : valid_deduction_guides) {
    REQUIRE_THAT(valid_deduction_guide,
                 CanParse(rules::user_class_template_deduction_guide,
                          "user_class_template_deduction_guide"));
  }
}

TEST_CASE("Parse valid method_signatures", "[method_signature]") {
  std::array valid_method_signatures{
      "int a()"s,
      "std::string s(int i)"s,
      "void s(const int i)"s,
      "void s(const int i) &"s,
      "void const_method(const int i) const"s,
      "constexpr void const_method(const int i) const"s,
      "void const_method_Rref(const int i) const &&"s,
      "void const_method_Rref(const int i) const && noexcept"s,
      "std::string no_name_params(int, float f, double)"s,
      "std::string no_name_params2(int, int , float f, double)"s,
      "rules::ast::val some_name(std::string s, int a)"s,
      "constexpr rules::ast::val some_name(std::string s, int a)"s,
      "auto& def_arg(int a = 2, long l = 3l)"s,
      "virtual auto& virtual_method(int a = 2, long l = 3l)"s,
      "virtual auto& virtual_method(int a = 2, long l = 3l) = 0"s,
      "virtual auto& virtual_method(int a = 2, long l = 3l) noexcept = 0"s,
      "template<typename T> auto def_arg(T a = 2, long l = 3l)"s,
      "template<typename T> auto overriden_method(T a = 2, long l = 3l) override"s,
      "template<typename T, int N = 0> auto def_arg(T a = 2, long l = 3l)"s,
      "const some_Type* function_a2(std::asd::ddd d)"s};

  for (auto& valid_method_signature : valid_method_signatures) {
    REQUIRE_THAT(valid_method_signature,
                 CanParse(rules::method_signature, "method_signature"));
  }
}

TEST_CASE("Parse valid operator_signatures", "[operator_signature]") {
  std::array valid_operator_signatures{
      "std::ostream& operator<<(std::ostream& os)"s,
      "void operator++()"s,
      "void operator++(int)"s,
      "void operator++(int) noexcept"s,
      "template<typename T> T operator*(T a, T b)"s,
      "template<typename T> T operator*(T a, T b) noexcept(sizeof(T) == 4)"s,
      "template<typename T> constexpr T operator*(T a, T b)"s,
      "void operator()(int)"s,
      "template <typename MetaDataMember> void operator()(MetaDataMember)"s,
      "void operator [ ] (int idx)"s,
      "virtual void operator [ ] (int idx) = 0"s,
      "bool operator>=(int a, int b)"s};

  for (auto& valid_operator_signature : valid_operator_signatures) {
    REQUIRE_THAT(valid_operator_signature,
                 CanParse(rules::operator_signature, "operator_signature"));
  }
}

TEST_CASE("Parse valid constructors and destructors", "[constructor]") {
  std::array valid_constructors{
      "a()"s,
      "s(int i)"s,
      "s(int i): a{i}"s,
      "constexpr s(int i): a{i}"s,
      "s(const int i)"s,
      "json_name( )"s,
      "json_name( T ptr )"s,
      "s(int i) noexcept"s,
      "s(int i) noexcept(true)"s,
      "s(int i) noexcept : a{i}"s,
      "asd(const asd & i)"s,
      "A(A&& i)"s,
      "A(int, float f, double)"s,
      "A(int, int , float f, double)"s,
      "A(int a, int b, float f) : a{a}, b(b), asd{f, a}"s,
      "~A()"s,
      "virtual ~A()"s,
      "virtual ~A() = 0"s,
      "virtual Asd(int a = 2, long l = 3l)"s,
      "template<typename T> def_arg(T a = 2, long l = 3l)"s,
      "template<typename T> overriden_method(T a = 2, long l = 3l)"s,
      "template<typename T, int N = 0> def_arg(T a = 2, long l = 3l)"s};

  for (auto& valid_constructor : valid_constructors) {
    REQUIRE_THAT(valid_constructor,
                 CanParse(rules::constructor, "constructor"));
  }
}

TEST_CASE("Parse valid class inheritance", "[inheritance]") {
  std::array valid_inheritances{": public A"s, ": public A, private B"s, ": A"s,
                                ": A, B"s, ": private A, public B"s};

  for (auto& valid_inheritance : valid_inheritances) {
    REQUIRE_THAT(valid_inheritance,
                 CanParse(rules::class_inheritances, "class inheritance"));
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
      "template <int N> class Test<N, int>"s,
      "template <class T> class Test<T, 3>"s,
      "template <> class Test<int, 3>"s,
      "struct point : A"s,
      "struct point : public A"s,
      "struct point : A, B"s,
      "struct point : A, protected B"s,
      "struct point : private A, private B"s,
  };

  for (auto& valid_class : valid_classes) {
    REQUIRE_THAT(valid_class,
                 CanParse(rules::class_or_struct, "class or struct"));
  }
}

TEST_CASE("Parse valid enum", "[enum]") {
  std::array valid_enums{
      "enum A"s,
      "enum A : bool"s,
      "enum class point"s,
      "enum class point: int"s,
  };

  for (auto& valid_enum : valid_enums) {
    REQUIRE_THAT(valid_enum, CanParse(rules::enumeration, "enum"));
  }
}
