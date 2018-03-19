#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

template <typename... Functions>
class Preprocessor {
  // Deduce the type of our parsers
  using me = Preprocessor<Functions...>;
  using my_ref = me&;
  using Parsers = std::tuple<std::invoke_result_t<Functions, my_ref>...>;

  inline static constexpr int number_of_parsers = sizeof...(Functions);

  // Compile time helper functions

  /*
   * Checks recursively if A and B parsers have different IDs
   */
  template <int A, int B>
  constexpr void check() {
    using first = std::tuple_element_t<A, Parsers>;
    using second = std::tuple_element_t<B, Parsers>;

    static_assert(first::id != second::id, "Parsers have the same ID");

    if constexpr (B + 1 < number_of_parsers) {
      check<A, B + 1>();
    } else if constexpr (A + 2 < number_of_parsers) {
      check<A + 1, A + 2>();
    }
  }

  /*
   * Starts a recursive check for parsers with duplicate IDs
   */
  constexpr void check_unique() {
    if constexpr (number_of_parsers > 1) {
      check<0, 1>();
    }
  }

  /*
   * Searches of a parser with the given ID
   *
   * Returns the parser's index if found, -1 otherwise
   */
  template <int ID, int N = 0>
  static constexpr int find_parser_with_id() {
    using to_check = std::tuple_element_t<N, Parsers>;

    if constexpr (to_check::id == ID) {
      return N;
    } else if constexpr (N + 1 < number_of_parsers) {
      return find_parser_with_id<ID, N + 1>();
    }

    return -1;
  }

  /*
   * Return the parser's index for a given ID, Not Found compiler error
   * otherwise
   */
  template <int ID>
  static constexpr int get_parsers_idx_with_error() {
    constexpr int idx = find_parser_with_id<ID>();
    static_assert(idx != -1, "Parser not found");

    return idx;
  }

  // Data members
  Parsers parsers;

 public:
  Preprocessor(Functions... funs) : parsers{funs(*this)...} { check_unique(); }

  // Methods

  /*
   * Check if a parser with given ID is present
   */
  template <int ID>
  constexpr bool has_parser_with_id() {
    return find_parser_with_id<ID>() != -1;
  }

  template <int ID>
  using parser =
      std::tuple_element_t<get_parsers_idx_with_error<ID>(), Parsers>;

  /*
   * Get a const reference to the parser with given PARSER_ID
   */
  template <int PARSER_ID>
  auto get_parser() -> parser<PARSER_ID> const& {
    constexpr int idx = find_parser_with_id<PARSER_ID>();
    return std::get<idx>(parsers);
  }
};

// TODO: remove below

template <typename Parent>
class B {
  Parent& parent;

 public:
  B(Parent& p) : parent{p} {}

  int foo() const { return 1; }

  constexpr static int id = 1;
};

template <typename Parent>
class C {
  Parent& parent;

 public:
  C(Parent& p) : parent{p} {}

  int baz() const { return 1; }

  constexpr static int id = 2;
};

int main() {
  auto l = [](auto& p) { return B{p}; };
  auto l2 = [](auto& p) { return C{p}; };
  Preprocessor a(l, l2);

  // B<int> because id does not depend on the template
  auto& b = a.get_parser<B<int>::id>();
  auto& c = a.get_parser<C<int>::id>();
  // auto& d = a.get_parser<3>(); // does not compile
  b.foo();
  c.baz();

  return 0;
}
