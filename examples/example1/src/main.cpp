#include <iostream>
#include <string>

#include <reflect.h>

struct Bar {
  int bazz;
  int foo;
};

int main() {
  std::cout << "hello from example\n";
  Bar obj{1, 2};

  // TODO: try some reflection when we can

  // using meta = reflect<Bar>;
  auto name = reflect::get_name<reflexpr<Bar>>;
  std::cout << "reflected name is " << name << std::endl;
  return 0;
}
