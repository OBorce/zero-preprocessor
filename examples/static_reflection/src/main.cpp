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
  using members = reflect::get_public_data_members_t<metaT>;
  constexpr auto size = reflect::get_size_v<members>;

  if constexpr (size == 0) {
    return true;
  }

  return compare_data_members<0, members, size>(a, b);
}

struct Bar {
  int bazz;
  int foo;

 private:
  std::string s;

 public:
  Bar(int a, int b, std::string s) : bazz{a}, foo{b}, s{std::move(s)} {}
};

template <class T>
struct A {
  int a;
  T t;
};

struct B : A<int> {
  int b;
};

enum class E {
  first, second
};

int main() {
  std::cout << "hello from example\n";
  // from foo.h
  Baz a{1, 2};
  Baz b{1, 3};

  bool equal = generic_equal(a, b);
  std::cout << "a == b : " << std::boolalpha << equal << std::endl;

  Bar bar{2, 4, "some string"};

  using meta = reflexpr<Bar>;

  auto class_name = reflect::get_name_v<meta>;
  std::cout << "reflected name is " << class_name << std::endl;

  using public_members = reflect::get_public_data_members_t<meta>;

  std::cout << "type " << class_name << " has "
            << reflect::get_size_v<public_members> << " public members "
            << std::endl;

  using first = reflect::get_element_t<0, public_members>;
  auto ptr1 = reflect::get_pointer_v<first>;

  std::cout << class_name << "::" << reflect::get_name_v<first> << " = "
            << bar.*ptr1 << std::endl;

  using second = reflect::get_element_t<1, public_members>;
  auto ptr2 = reflect::get_pointer_v<second>;

  std::cout << class_name << "::" << reflect::get_name_v<second> << " = "
            << bar.*ptr2 << std::endl;

  using all_members = reflect::get_data_members_t<meta>;
  std::cout << "type " << class_name << " has "
            << reflect::get_size_v<all_members> << " data members "
            << std::endl;

  using third = reflect::get_element_t<2, all_members>;
  auto ptr3 = reflect::get_pointer_v<third>;
  std::cout << class_name << "::" << reflect::get_name_v<third> << " = "
            << bar.*ptr3 << std::endl;

  // inheritance
  using metaB = reflexpr<B>;
  using bases = reflect::get_public_base_classes_t<metaB>;
  auto name = reflect::get_name_v<metaB>;
  std::cout << "type " << name << " has "
            << reflect::get_size_v<bases> << " public bases " << std::endl;

  using base1 = reflexpr<reflect::get_element_t<0, bases>>;
  std::cout << "type " << name << " inherits from "
            << reflect::get_name_v<base1> << std::endl;

  // enums

  using E_m = reflexpr<E>;
  using first_m = reflect::get_element_t<0, reflect::get_enumerators_t<E_m>>;
  std::cout << reflect::get_name_v<first_m> << std::endl; // prints "first"

  return 0;
}
