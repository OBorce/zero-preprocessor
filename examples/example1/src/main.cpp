#include <iostream>
#include <string>

#include <reflect.h>

template <typename T>
struct compare_data_members {
  const T& a;
  const T& b;
  bool& result;

  /*
  template <typename MetaDataMember>
  void operator()(MetaDataMember) const {
    auto mem_ptr = reflect::get_pointer_v<MetaDataMember>;
    result &= a.*mem_ptr == b.*mem_ptr;
  }
  */
};

/*
template <typename T>
bool generic_equal(const T& a, const T& b) {
  using metaT = reflexpr<T>;
  bool result = true;
  reflect::for_each<reflect::get_data_members_t<metaT>>(
      compare_data_members<T>{a, b, result});

  return result;
}
*/

struct Bar {
  int bazz;
  int foo;
};

int main() {
  std::cout << "hello from example\n";
  Bar obj{1, 5};

  // TODO: try some reflection when we can

  using meta = reflexpr<Bar>;

  auto name = reflect::get_name<meta>;
  std::cout << "reflected name is " << name << std::endl;

  using members = reflect::get_public_members<meta>;

  auto ptr1 = reflect::get_pointer_v<std::tuple_element_t<0, members>>;

  std::cout << "Bar::bazz = " << obj.*ptr1 << std::endl;

  auto ptr2 = reflect::get_pointer_v<std::tuple_element_t<1, members>>;

  std::cout << "Bar::foo = " << obj.*ptr2 << std::endl;
  return 0;
}
