#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#include <std_parser.hpp>

#include <catch2/catch.hpp>

using std::string_literals::operator""s;
using namespace std_parser;

class CanParse : public Catch::MatcherBase<std::string> {
  std::string rule_name;
  std_parser::CodeFragment setup;

 public:
  CanParse(std::string_view name,
           std_parser::CodeFragment setup = rules::ast::Namespace{"nsp"}) :
      rule_name{name}, setup{std::move(setup)} {}

  // Performs the test for this matcher
  virtual bool match(std::string const& str) const override {
    StdParserState parser;
    std_parser::CodeFragment tmp = setup;
    parser.open_new_code_fragment(std::move(tmp));

    using Iter = decltype(str.begin());
    struct {
      Iter begin_;
      Iter end_;

      // TODO: maybe add an advance function
      std::uint16_t row = 0;
      std::uint16_t col = 0;

      auto get_row() { return row; }
      auto get_column() { return col; }

      Iter begin() { return begin_; }
      Iter end() { return end_; }

      explicit operator bool() const { return begin_ != end_; }
    } source{str.begin(), str.end()};

    while (source) {
      if (auto out = parser.parse(source)) {
        source.begin_ = out->processed_to;
      } else {
        std::string left{source.begin_, source.end_};
        auto& c = parser.get_all_code_fragments();
        std::cout << "inside " << c.size() << " can't parse " << left << std::endl;
        for (auto& v : c) {
          std::visit(
              [&](auto const& c) { std::cout << c.loc.row << c.loc.col << '\n'; },
              v);
          std::visit(overloaded{[&](rules::ast::Namespace const&) {
                                  std::cout << "namespace " << std::endl;
                                },
                                [&](rules::ast::Class const&) {
                                  std::cout << "class " << std::endl;
                                },
                                [&](rules::ast::Function const&) {
                                  std::cout << "function " << std::endl;
                                },
                                [&](rules::ast::Scope const&) {
                                  std::cout << "local scope " << std::endl;
                                },
                                [&](rules::ast::Enumeration const&) {
                                  std::cout << "enum " << std::endl;
                                },
                                [&](rules::ast::Statement const&) {
                                  std::cout << "Statement " << std::endl;
                                },
                                [&](rules::ast::Expression const& arg) {
                                  std::cout << "expression " << arg.is_begin()
                                            << std::endl;
                                },
                                [&](rules::ast::RoundExpression const& arg) {
                                  std::cout << "round expression " << arg.is_begin()
                                            << std::endl;
                                },
                                [&](rules::ast::CurlyExpression const& arg) {
                                  std::cout << "curly expression " << arg.is_begin()
                                            << std::endl;
                                },
                                [&](rules::ast::Vars const& arg) {
                                  std::cout << "vars "
                                            << static_cast<int>(arg.state)
                                            << std::endl;
                                },
                                [&](rules::ast::IfStatement const& arg) {
                                  std::cout << "if expression "
                                            << static_cast<int>(arg.state)
                                            << std::endl;
                                },
                                [&](rules::ast::FunctionDeclaration const&) {
                                  std::cout << "function decl " << std::endl;
                                },
                                [](auto&) {}},
                     v);
        }
        return false;
      }
    }

    return true;
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "Trying to be parsed as a/an " << rule_name;
    return ss.str();
  }
};

TEST_CASE("Parse valid variables", "[var]") {
  std::array valid_vars{"int a;"s,
                        "std::string s{\"hello\"};"s,
                        "std::vector<int> v;"s,
                        "int i = 0;"s,
                        "int i, j;"s,
                        "int i = 2, j = 3;"s,
                        "int i, j = 3;"s,
                        "int i = 2, j;"s,
                        "int i = 2 + 2, j;"s,
                        "int i = foo(2);"s,
                        "int i = foo(2) + 2 / 3 * (foo(a + 2, 3) - 4);"s,
                        "int i = 2, j {3};"s,
                        "auto i = [] { return 0;};"s,
                        "auto i = [] (int j = 2) { return j + 2;};"s,
                        "auto i = [] (int j = 2) { return j + 2;}(2);"s,
                        "int i, j;"s,
                        "auto mem_ptr = reflect::get_pointer_v<currentMember>;"s,
                        "std::vector<int> v {};"s,
                        "std::vector<int> v {1, 2, 3};"s,
                        "std::pair<int, float> v {1, 2.0f};"s,
                        "std::pair<int, std::vector<char>> v {{}};"s,
                        "std::pair<int, std::vector<char>> v {1, {}};"s,
                        "std::pair<std::vector<char>, int> v {{}, 2};"s,
                        "std::pair<int, std::vector<char>> v {{}, {}};"s,
                        "std::array<int, 4> a;"s,
                        "rules::ast::val v = 2;"s,
                        "rules::ast::val v = {2, foo(a)};"s,
                        "nsd::asd::varr v23 {2, 3 / 2, baz(1, 3)};"s,
                        "some_Type var_a2 = foo(2) ;"s};

  for (auto& valid_var : valid_vars) {
    REQUIRE_THAT(valid_var, CanParse("variable", rules::ast::Function{}));
  }
}

TEST_CASE("Parse valid if statement", "[if_statement]") {
  std::array valid_if_statements{
      "if (true) {}"s,
      "if(a || b) a = 2;"s,
      "if(a || !b) a = 3;"s,
      "if (!asd) {}"s,
      "if (a.foo()) {}"s,
      "if constexpr (a && b) {}"s,
      "if (a.foo([](int i) { return i;})) {}"s,
      "if (bool b = foo()) {}"s,
      "if (bool b = foo()) return 2;"s,
      "if(false) a = 2; else {}"s,
      "if(false) a = 2; else a = 3;"s,
      "if(false) a = 2; else if(false) a = 3;"s,
      "if(auto foo = make_foo(); foo == 2) a = 2; else if(false) a = 3;"s,
      "if(false) a = 2; else if(false) a = 3; else a = 4;"s,
      "if(false) return 2; else if(false) return 3; else return 4;"s,
      "if (int i = foo(); i) {}"s};

  for (auto& valid_if_statement : valid_if_statements) {
    REQUIRE_THAT(valid_if_statement,
                 CanParse("if_statement", rules::ast::Function{}));
  }
}

TEST_CASE("Parse valid expression", "[expression]") {
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
                               "foo(a...)"s,
                               "a - b"s,
                               "a && !b"s,
                               "a.foo()"s,
                               "a.foo(i, std::string{})"s,
                               "a * b"s};

  for (auto& valid_expression : valid_expressions) {
    REQUIRE_THAT(valid_expression,
                 CanParse("expression", rules::ast::Expression{}));
  }
}

TEST_CASE("Parse valid parameters", "[parameters]") {
  std::array valid_function_signitures{
      "int"s,
      "int i"s,
      "int = 2"s,
      "int i = 2"s,
      "int, int"s,
      "int, int i"s,
      "int, int = 2"s,
      "int, int i = 2"s,
      "int x, int"s,
      "int x, int i"s,
      "int x, int = 2"s,
      "int x, int i = 2"s,
      "int = 1, int"s,
      "int = 1, int i"s,
      "int = 1, int = 2"s,
      "int = 1, int i = 2"s,
      "int, float f, double"s,
      "int a, long l = 3l"s,
      "int a = 2, long"s,
      "int a = 2, long l"s,
      "int a = 2, long l = 3l"s,
      "int a, long l = 3l, int x"s,
  };

  for (auto& valid_fun_decl : valid_function_signitures) {
    REQUIRE_THAT(valid_fun_decl, CanParse("parameters", rules::ast::Params{}));
  }
}

TEST_CASE("Parse valid function declaration", "[function_declaration]") {
  std::array valid_function_signitures{
      "int a();"s,
      "std::string s(int i);"s,
      "void s(const int i);"s,
      "constexpr int s(const int i);"s,
      "void s(const int i) noexcept;"s,
      "std::string no_name_params(int, float f, double);"s,
      "std::string no_name_params2(int, int , float f, double);"s,
      "rules::ast::val some_name(std::string s, int a);"s,
      "auto& def_arg(int a = 2, long l = 3l);"s,
      "template<typename T> auto def_arg(T a = 2, long l = 3l);"s,
      "template<typename T> auto def_arg(T a = 2, long l = 3l) noexcept(sizeof(T) > 2);"s,
      "template<typename T, int N = 0> auto def_arg(T a = 2, long l = 3l);"s,
      "const some_Type* function_a2(std::asd::ddd d);"s};

  for (auto& valid_fun_decl : valid_function_signitures) {
    REQUIRE_THAT(valid_fun_decl,
                 CanParse("function_declaration", rules::ast::Namespace{"nsp"}));
  }
}
