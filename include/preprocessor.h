#include <functional>
#include <iterator>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

template <typename Source, typename... Functions>
class Preprocessor {
  // Deduce the type of our parsers
  using me = Preprocessor<Source, Functions...>;
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

  /**
   * process the source with the ID-th parser
   *
   * Return the cursor to the end of the successfully parsed source
   * if has output else try to parse the source using the next parser
   *
   * Throws runtime_error if none of the parsers can parse the source
   */
  template <int ID>
  auto process(Source& source) {
    auto out = std::get<ID>(parsers).parse(source);
    if (out) {
      return *out;
    }

    if constexpr (ID + 1 < number_of_parsers) {
      return process<ID + 1>(source);
    }

    // TODO: show part of the unparsable source
    throw std::runtime_error("source can't be parsed by none of the parsers");
  }

  void process_source(Source& source) {
    while (!source.is_finished()) {
      auto processed_to = process<0>(source);
      size_t processed_chars = std::distance(source.begin(), processed_to);
      if (processed_chars == 0) {
        throw std::runtime_error("error in one of the parsers");
      }

      source.advance_for(processed_chars);
    }
  }

  // Data members
  std::vector<Source> sources;
  Parsers parsers;

 public:
  Preprocessor(std::vector<Source>&& sources, Functions... funs)
      : sources{std::move(sources)}, parsers{funs(*this)...} {
    check_unique();
  }

  // Methods

  void process() {
    for (auto& source : sources) {
      process_source(source);
    }
  }

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
