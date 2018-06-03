#include <iostream>
#include <string>
#include <tuple>

constexpr void interface(meta::type target, const meta::type source) {
  compiler.require(source.variables().empty(),
                   "interfaces may not contain data");
  for (auto f : source.functions()) {
    compiler.require(
        !f.is_copy() && !f.is_move(),
        "interfaces may not copy or move; consider a virtual clone() instead");
    if (!f.has_access()) f.make_public();
    compiler.require(f.is_public(), "interface functions must be public");
    f.make_pure_virtual();
    ->(target)f;
  }
  ->(target){virtual ~source.name()$() noexcept {}};
};

interface shape {
  int get_area();
};

struct square : shape {
  int a = 2;

  int get_area() { return a * a; }
};

int main() {
  std::cout << "hello from example\n";

  square s;
  std::cout << " area of the square s is : " << s.get_area() << std::endl;

  return 0;
}
