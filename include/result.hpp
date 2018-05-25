#ifndef RESULT_H
#define RESULT_H

#include <iterator>

template <typename T>
using Iter = decltype(std::declval<T>().begin());

template <typename T>
using IterValue = typename std::iterator_traits<Iter<T>>::value_type;

template <typename T>
using SourceValue = typename std::iterator_traits<T>::value_type;

template <typename SourceIter, typename Iterable>
struct Result {
  SourceIter processed_to;
  Iterable result;

  Result(SourceIter processed_to, Iterable it)
      : processed_to{processed_to}, result{it} {
    static_assert(
        std::is_same<SourceValue<SourceIter>, IterValue<Iterable>>(),
        "Provided iterable and source iterator are of dirrerent types");
  }
};

#endif  //! RESULT_H
