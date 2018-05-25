#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <detect.hpp>
#include <result.hpp>
#include <source_loader.hpp>

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

  template <typename Source>
  auto throw_unparsable_error(Source& source) {
    std::size_t remaining = std::distance(source.begin(), source.end());
    std::size_t size = std::min(30ul, remaining);
    std::string_view content = {&*source.begin(), size};

    std::string error_msg = "source can't be parsed by none of the parsers: ";
    error_msg += std::to_string(content.size());
    error_msg += '\n';
    error_msg += content;
    return std::runtime_error(error_msg);
  }

  /**
   * process the source with the N-th parser
   * If successful write the result using the writer
   *
   * Return the cursor to the end of the successfully parsed source
   * if has output else try to parse the source using the next parser
   *
   * Throws runtime_error if none of the parsers can parse the source
   */
  template <int N = 0, typename Source, typename Writer>
  auto process(Source& source, Writer& writer) {
    std::cout << "processing with " << N << "th parser\n";
    auto out = std::get<N>(parsers).parse(source);
    if (out) {
      writer((*out).result);
      return (*out).processed_to;
    }

    if constexpr (N + 1 < number_of_parsers) {
      return process<N + 1>(source, writer);
    }

    throw throw_unparsable_error(source);
  }

  template <class T, class Source>
  using preprocess_fun =
      decltype(std::declval<T>().preprocess(std::declval<Source>()));

  /**
   * Pre-process a source by all of the parsers that need to pre-process their
   * target
   *
   * NOTE: only one parser can process a statement!
   *
   * Return the cursor to the end of the successfully parsed source
   * if successful else try to parse the source using the next parser
   *
   * Throws runtime_error if none of the parsers can parse the source
   */
  template <int N = 0, typename Source>
  auto preprocess(Source& source) {
    if constexpr (is_detected_v<preprocess_fun, parser_type<N>, Source&>) {
      auto out = std::get<N>(parsers).preprocess(source);
      if (out) {
        return (*out).processed_to;
      }
    }

    // TODO: test if there are more pre parsers with preprocess
    if constexpr (N + 1 < number_of_parsers) {
      return preprocess<N + 1>(source);
    }

    std::cout << "trying to parse with std\n";
    constexpr int parser_idx = get_parsers_idx_with_error<std_parser_id>();
    auto out = std::get<parser_idx>(parsers).parse(source);
    if (out) {
      return (*out).processed_to;
    }

    throw throw_unparsable_error(source);
  }

  template <class T>
  using finish_preprocess_fun = decltype(std::declval<T>().finish_preprocess());

  /**
   * Call finish_preprocess on all parsers that have one
   */
  template <int N = 0>
  void finish_preprocess() {
    if constexpr (is_detected_v<finish_preprocess_fun, parser_type<N>>) {
      std::get<N>(parsers).finish_preprocess();
    }

    if constexpr (N + 1 < number_of_parsers) {
      finish_preprocess<N + 1>();
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
  source::SourceLoader source_loader;
  Parsers parsers;

 public:
  Preprocessor(source::SourceLoader&& loader, Functions... funs)
      : source_loader{std::move(loader)}, parsers{funs(*this)...} {
    check_unique();
    constexpr int idx = find_parser_with_id<std_parser_id>();
    static_assert(idx != -1, "STD Parser not found");
    init();
  }

  inline static constexpr int std_parser_id = 's' + 't' + 'd';

  // Methods

  /**
   * Send the source through the parsers for processing until it is finished
   *
   * Throws runtime error if none of the parsers can process the source
   * or a parser reported that it processed 0 length of the source
   */
  template <typename Writer>
  void process_source(std::string_view source_name, Writer& writer) {
    std::cout << " process source " << source_name << "\n";
    auto source = source_loader.load_source(source_name);
    while (!source.is_finished()) {
      auto processed_to = process(source, writer);

      std::size_t processed_chars = std::distance(source.begin(), processed_to);
      if (processed_chars == 0) {
        throw std::runtime_error("error in one of the parsers");
      }

      std::cout << "processed " << processed_chars << " chars\n";

      source.advance(processed_chars);
    }
  }

  /**
   * Write the dependencies of the source using the writer
   *
   * Terminates if a found dependency file can't be opened
   */
  template <typename Writer>
  void get_dependencies(std::string source_name, Writer& writer) {
    std::unordered_set<std::string> dependencies;
    std::vector<std::string> sources = {std::move(source_name)};
    constexpr int parser_idx = get_parsers_idx_with_error<std_parser_id>();
    auto& std_parser = std::get<parser_idx>(parsers);
    while (!sources.empty()) {
      auto& name = sources.back();
      auto source = source_loader.load_source(name);
      // process source
      auto deps = std_parser.get_includes(source);
      // remove processed
      sources.pop_back();

      // add new to be processed
      for (auto& dep : deps) {
        if (source::is_standard(dep)) {
          continue;
        }

        if (auto include = source_loader.find_source(dep); include) {
          auto name = include.value().string();
          auto [it, ok] = dependencies.emplace(name);
          if (ok) {
            // write the dependencies using the writer
            writer(name);
            auto out = source_loader.get_out_path(dep).string();
            writer(out);

            sources.push_back(std::move(name));
          }
        } else {
          std::cout << "file " << dep << " can't be found\n";
        }
      }
    }
  }

  void preprocess_source(std::string_view source_name) {
    auto source = source_loader.load_source(source_name);
    // TODO: work with headers! And need start_preprocessing?
    while (!source.is_finished()) {
      auto processed_to = preprocess(source);

      std::size_t processed_chars = std::distance(source.begin(), processed_to);
      if (processed_chars == 0) {
        throw std::runtime_error("error in one of the parsers");
      }

      source.advance(processed_chars);
    }

    finish_preprocess();
  }

  /*
   * Check if a parser with given ID is present
   */
  template <int ID>
  constexpr bool has_parser_with_id() {
    return find_parser_with_id<ID>() != -1;
  }

  template <int ID>
  using parser_by_id =
      std::tuple_element_t<get_parsers_idx_with_error<ID>(), Parsers>;

  /*
   * Get a reference to the parser with given PARSER_ID
   */
  template <int PARSER_ID>
  auto get_parser() -> parser_by_id<PARSER_ID>& {
    constexpr int idx = find_parser_with_id<PARSER_ID>();
    return std::get<idx>(parsers);
  }
};
#endif  // PREPROCESSOR_H
