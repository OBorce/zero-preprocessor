#include <iostream>
#include <string>

struct Bar {
  int bazz;
  std::string name;
};

int main() {
  std::cout << "hello from example\n";
  Bar obj;

  // TODO: try some reflection when we can
  return 0;
}
