#ifndef SOURCE_H
#define SOURCE_H

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

// TODO: make the Source work with files or iterators
class Source {
  // Data members

  // TODO: for now just a string representing the source code
  const std::string source;
  const std::string name;

  std::size_t processed_till = 0;

  std::uint16_t rows_processed = 1;
  std::uint16_t column = 0;

 public:
  // Constructor
  Source(std::string_view source, std::string_view name)
      : source{source}, name{name} {}

  // Methods

  /**
   * Has the entire source been processed
   */
  bool is_finished() { return processed_till == source.size(); }

  // allow us to for loop the unprocessed part of the source

  auto begin() const { return source.cbegin() + processed_till; }

  auto end() const { return source.cend(); }

  auto get_row() const { return rows_processed; }

  auto get_column() const { return column; }

  /**
   * Advances for num_characters
   * marks the number of characters as processed
   */
  void advance(std::size_t num_characters) {
    constexpr char new_line = '\n';
    auto from = begin();
    auto to = from + num_characters;
    rows_processed += std::count(from, to, new_line);

    auto r_to = std::make_reverse_iterator(from);
    auto r_from = std::make_reverse_iterator(to);
    column = std::distance(r_from, std::find(r_from, r_to, new_line)) + 1;

    processed_till += num_characters;
  }

  auto& get_name() { return name; }
};
#endif  //! SOURCE_H
