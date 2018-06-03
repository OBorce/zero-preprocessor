#include <iostream>
#include <string>
#include <tuple>

#include <foo.h>
#include <reflect.hpp>

template <std::size_t N, typename Members, std::size_t Size, typename T>
bool compare_data_members(const T& a, const T& b) {
  using currentMember = reflect::get_element_t<N, Members>;
  auto mem_ptr = reflect::get_pointer_v<currentMember>;

  bool result = a.*mem_ptr == b.*mem_ptr;
  if (!result) {
    return result;
  }

  constexpr auto next = N + 1;
  if constexpr (next < Size) {
    return compare_data_members<next, Members, Size>(a, b);
  }

  return true;
};

template <typename T>
bool generic_equal(const T& a, const T& b) {
  using metaT = reflexpr<T>;
  using members = reflect::get_public_data_members<metaT>;
  constexpr auto size = reflect::get_size_v<members>;

  if constexpr (size == 0) {
    return true;
  }

  return compare_data_members<0, members, size>(a, b);
}

struct Bar {
  int bazz;
  int foo;
};

template <class T>
struct A {
  int a;
  T t;
};

struct B : A<int> {
  int b;
};

int main() {
  std::cout << "hello from example\n";
  Bar a{1, 2};
  Bar b{1, 3};

  bool equal = generic_equal(a, b);

  std::cout << "a == b : " << std::boolalpha << equal << std::endl;

  using meta = reflexpr<Bar>;

  auto name = reflect::get_name_v<meta>;
  std::cout << "reflected name is " << name << std::endl;

  using members = reflect::get_public_data_members<meta>;

  std::cout << "type " << name << " has "
            << reflect::get_size_v<members> << " public members " << std::endl;

  auto ptr1 = reflect::get_pointer_v<reflect::get_element_t<0, members>>;

  std::cout << "Bar::bazz = " << a.*ptr1 << std::endl;

  auto ptr2 = reflect::get_pointer_v<reflect::get_element_t<1, members>>;

  std::cout << "Bar::foo = " << a.*ptr2 << std::endl;

  // inheritance
  using metaB = reflexpr<B>;
  using bases = reflect::get_public_base_classes<metaB>;
  std::cout << "type " << name << " has "
            << reflect::get_size_v<bases> << " public bases " << std::endl;

  using base1 = reflexpr<reflect::get_element_t<0, bases>>;
  std::cout << "type " << name << " inherits from "
            << reflect::get_name_v<base1> << std::endl;

  return 0;
}
