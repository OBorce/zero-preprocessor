#ifndef HEAP_OBJ_HPP
#define HEAP_OBJ_HPP

#include <memory>

template <typename T>
struct HeapObj : private std::unique_ptr<T> {
  HeapObj() = default;
  HeapObj(HeapObj<T> const& h)
      : std::unique_ptr<T>{h != nullptr ? std::make_unique<T>(*h)
                                              : nullptr} {}
  HeapObj(HeapObj<T>&& h) : std::unique_ptr<T>{std::move(h)} {}

  HeapObj(T&& value)
      : std::unique_ptr<T>{std::make_unique<T>(std::move(value))} {}

  HeapObj<T>& operator=(HeapObj<T>&& h) {
    std::unique_ptr<T>::operator=(std::move(h));
    return *this;
  }

  HeapObj<T>& operator=(HeapObj<T> const& h) {
    if (*this) {
      if (h) {
        **this = *h;
      } else {
        this->reset();
      }
    } else if (h) {
      std::unique_ptr<T>::operator=(std::make_unique<T>(*h));
    }

    return *this;
  }

  HeapObj<T>& operator=(T&& value) {
    if (*this) {
      **this = std::move(value);
    } else {
      std::unique_ptr<T>::operator=(std::make_unique<T>(std::move(value)));
    }
    return *this;
  }
};

#endif  //! HEAP_OBJ_HPP
