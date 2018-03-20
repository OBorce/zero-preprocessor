#include <algorithm>
#include <string>
#include <string_view>

class Source {
  // Data members

  // TODO: for now just a string representing the source code
  const std::string source;

  size_t processed_till = 0;
  size_t advance_to = 0;

 public:
  // Methods

  const std::string_view read_line() {
    auto from = source.data() + processed_till;
    auto end = source.data() + source.size();
    auto found = std::find(from, end, '\n');
    if (found == end) {
      advance_to = source.size();
      return {from, source.size() - processed_till};
    }

    size_t distance = found - from + 1;
    advance_to = processed_till + distance;
    return {from, distance};
  }

  /**
   * Has the entire source been processed
   */
  bool is_finished() { return processed_till == source.size(); }

  /**
   * Advances to the end of the last read segment
   * marks the read segment as processed
   */
  void advance() { processed_till = advance_to; }

  /**
   * Advances for num_characters
   * marks the number of characters as processed
   */
  void advance_for(size_t num_characters) { processed_till += num_characters; }

  // allow us to for loop the unprocessed part of the source

  auto begin() { return source.cbegin() + processed_till; }

  auto end() { return source.cend(); }
};
