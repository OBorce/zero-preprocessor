#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <detect.h>
#include <result.h>

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
   * If successful write the result using the writer
   *
   * Return the cursor to the end of the successfully parsed source
   * if has output else try to parse the source using the next parser
   *
   * Throws runtime_error if none of the parsers can parse the source
   */
  template <int ID, typename Writer>
  auto process(Source& source, Writer& writer) {
    std::size_t remaining = std::distance(source.begin(), source.end());
    std::size_t size = std::min(30ul, remaining);
    std::string_view content = {&*source.begin(), size};

    auto out = std::get<ID>(parsers).parse(source);
    if (out) {
      writer((*out).result);
      return (*out).processed_to;
    }

    if constexpr (ID + 1 < number_of_parsers) {
      return process<ID + 1>(source, writer);
    }

    std::string error_msg = "source can't be parsed by none of the parsers: ";
    error_msg += content;
    throw std::runtime_error(error_msg);
  }

  /**
   * Send the source through the parsers for processing until it is finished
   *
   * Throws runtime error if none of the parsers can process the source
   * or a parser reported that it processed 0 length of the source
   */
  template <typename Writer>
  void process_source(Source& source, Writer& writer) {
    while (!source.is_finished()) {
      auto processed_to = process<0>(source, writer);

      std::size_t processed_chars = std::distance(source.begin(), processed_to);
      if (processed_chars == 0) {
        throw std::runtime_error("error in one of the parsers");
      }

      source.advance(processed_chars);
    }
  }

  template <class T>
  using onInit = decltype(std::declval<T>().onInit());

  template <int N>
  using parser_type = std::tuple_element_t<N, Parsers>;

  /**
   * Call onInit for all parsers that have one
   */
  template <int N = 0>
  void init() {
    if constexpr (is_detected_v<onInit, parser_type<N>>) {
      std::get<N>(parsers).onInit();
    }

    if constexpr (N + 1 < number_of_parsers) {
      return init<N + 1>();
    }
  }

  // Data members
  std::vector<Source> sources;
  Parsers parsers;

 public:
  Preprocessor(std::vector<Source>&& sources, Functions... funs)
      : sources{std::move(sources)}, parsers{funs(*this)...} {
    check_unique();
    init();
  }

  // Methods

  template <typename Writer>
  void process(Writer& writer) {
    for (auto& source : sources) {
      process_source(source, writer);
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
   * Get a reference to the parser with given PARSER_ID
   */
  template <int PARSER_ID>
  auto get_parser() -> parser<PARSER_ID>& {
    constexpr int idx = find_parser_with_id<PARSER_ID>();
    return std::get<idx>(parsers);
  }
};
#endif  //! PREPROCESSOR_H
