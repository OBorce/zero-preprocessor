#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string_view>

// FIXME: this only works for char
// TODO: change this to a span<T> in 2a
template <typename Iter>
std::string_view make_string_view(Iter begin, Iter end) {
  std::size_t size = std::distance(begin, end);
  return {std::addressof(*begin), size};
}

#endif  //! STRING_UTILS_H
