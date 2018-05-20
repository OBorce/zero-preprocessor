#include <tuple>

template <typename T>
struct Data {};

namespace meta {
template <typename T, int N = 0>
struct object {
  static auto valueOf() {
    if constexpr (!is_null()) {
      return std::get<N>(Data<T>::funs);
    }
  }

  static constexpr bool is_null() {
    return N == std::tuple_size<decltype(Data<T>::funs)>();
  }

  static auto next() { return object<T, N + 1>{}; }
};

template <typename T>
constexpr bool is_null() {
  return T::is_null();
}
}  // namespace meta

struct Foo {
  int a;
  int b;
  int c;
};

template <typename T>
constexpr auto reflexpr() {
  return meta::object<T>{};
}

template <typename T>
using next = decltype(T::next());
#define next(A) next<A>
#define valueof(A) A::valueOf()

template <>
struct Data<Foo> {
  constexpr inline static std::tuple funs = {&Foo::a, &Foo::b, &Foo::c};
};

template <class T, class K>
auto call(K k) {
  if constexpr (!meta::is_null<T>()) {
    auto sum = k.*valueof(T);
    return sum + call<next(T)>(k);
  }

  return 0;
}

template <class T>
auto fooo(T f) {
  constexpr auto reflex = meta::object<T>{};
  return call<decltype(reflex)>(f);
}

template <class X, typename T>
bool compare(const T& a, const T& b) {
  if constexpr (!meta::is_null<X>()) {
    // if constexpr (meta::is_data_member(X)) {
    auto p = valueof(X);
    if (a.*p != b.*p) return false;
    //}

    return compare<next(X)>(a, b);
  }

  return true;
}

template <typename T>
bool equal(const T& a, const T& b) {
  constexpr meta::object type = reflexpr<T>();
  // constexpr auto first = meta::first_member(type);
  return compare<decltype(type)>(a, b);
}

int main() {
  Foo f{1, 2, 1};
  Foo f2{1, 2, 2};
  auto rez = fooo(f);

  auto e = equal(f, f2);
  return e + rez;
}
