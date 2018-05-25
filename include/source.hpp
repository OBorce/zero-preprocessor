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

 public:
  // Constructor
  Source(std::string_view source, std::string_view name)
      : source{source}, name{name} {}

  // Methods

  /**
   * Has the entire source been processed
   */
  bool is_finished() { return processed_till == source.size(); }

  /**
   * Advances for num_characters
   * marks the number of characters as processed
   */
  void advance(std::size_t num_characters) { processed_till += num_characters; }

  // allow us to for loop the unprocessed part of the source

  auto begin() { return source.cbegin() + processed_till; }

  auto end() { return source.cend(); }

  auto& get_name() { return name; }
};
#endif  //! SOURCE_H
