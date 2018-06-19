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
  ->(target){virtual ~source.name()$() noexcept {}}
};

constexpr void base_class(meta::type target, const meta::type source) {
  for (auto f : source.functions()) {
    if (f.is_destructor() && !((f.is_public() && !f.is_virtual()) ||
                               (f.is_protected() && !f.is_virtual()))) {
      compiler.error(
          "base class destructors must be public and virtual, or protected and "
          "nonvirtual");
    }

    compiler.require(!f.is_copy() && !f.is_move(),
                     "base classes may not copy or move; consider a virtual "
                     "clone() instead");
    if (!f.has_access()) f.make_public();
    ->(target)f;
  }

  for (auto b : source.bases()) {
    if (!b.has_access()) b.make_public();
    ->(target)b;
  }

  compiler.require(source.variables().empty(),
                   "pure base classes may not contain data");
}


constexpr void basic_value(meta::type target, const meta::type source) {
  for (auto m : source.members_and_bases()) {
    ->(target)m;
  }

  for(auto f : source.functions()) {
    if (f.is_default_ctor()) {
      ->(target) { source.name()$() = default; }
    }
    if (f.is_copy_ctor()) {
      ->(target) { source.name()$(const source.name()$ & that) = default; }
    }
    if (f.is_move_ctor()) {
      ->(target) { source.name()$(source.name()$ && that) = default; }
    }
    if (f.is_copy_assignment()) {
      ->(target) { source.name()$ & operator=(const source.name()$ & that) = default; }
    }
    if (f.is_move_assignment()) {
      ->(target) { source.name()$ & operator=(source.name()$ && that) = default; }
    }
  }

  for (auto f : source.functions()) {
    compiler.require(
        !f.is_protected() && !f.is_virtual(),
        "a value type must not have a protected or virtual function");
    compiler.require(!f.is_destructor() || !f.is_public(), "a value type must have a public destructor");
  }
}

interface shape {
  int get_area() const;

  int some_formula(int const x);
};

base_class Rectangle : shape {
  int get_area() const override { return 2; }

  int some_formula(int const x) override { return x * x - 2; }
};

struct square : shape {
  int a;

  square(int a) : a{a} {}

  int get_area() const override { return a * a; }

  int some_formula(int const x) override { return x * x - a * a; }
};

basic_value point {
  float x;
  float y;
};

int main() {
  std::cout << "hello from example\n";

  square s{3};
  std::cout << " area of the square s is : " << s.get_area() << std::endl;

  return 0;
}
