#include <tuple>

template <typename T>
struct Data {};

template <typename T>
constexpr auto reflexpr() {
  return Data<T>{};
}
#define reflexpr(T) reflexpr<T>()

namespace meta {
using object = int64_t;

constexpr object first_member(object o) {
  // TODO: reset first Members bits
  return o + 1;
}

constexpr bool is_null(object o) { return o == 0; }
}  // namespace meta

struct Foo {
  int a;
  int b;
  int c;
};

using types = std::tuple<Foo>;

template <meta::object o>
constexpr auto valueof() {
  constexpr int T = (o >> 10) - 1;
  using D = Data<typename std::tuple_element<T, types>::type>;
  constexpr int N = (o & ((1 << 10) - 1)) - 1;
  return std::get<N>(D::funs);
}
#define valueof(T) valueof<T>()

template <meta::object o>
constexpr meta::object next() {
  constexpr int T = (o >> 10) - 1;
  using D = Data<typename std::tuple_element<T, types>::type>;
  constexpr int N = (o & ((1 << 10) - 1)) - 1;
  return N + 1 < std::tuple_size<decltype(D::funs)>::value ? o + 1 : 0;
}
#define next(T) next<T>()

template <meta::object T, class K>
auto call(K k) {
  if constexpr (!meta::is_null(T)) {
    auto sum = k.*valueof(T);
    return sum + call<next(T)>(k);
  }

  return 0;
}

template <class T>
auto fooo(T f) {
  constexpr auto type = reflexpr(T);
  constexpr auto first = meta::first_member(type);
  return call<first>(f);
}

template <meta::object X, typename T>
bool compare(const T& a, const T& b) {
  if constexpr (!meta::is_null(X)) {
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
  constexpr meta::object type = reflexpr(T);
  constexpr meta::object first = meta::first_member(type);
  return compare<first>(a, b);
}

template <>
struct Data<Foo> {
  constexpr inline static std::tuple funs = {&Foo::a, &Foo::b, &Foo::c};

  constexpr operator int64_t() const { return 1 << 10; }
};

int main() {
  Foo f{1, 2, 1};
  Foo f2{1, 2, 1};
  auto rez = fooo(f);

  auto e = equal(f, f2) + rez;
  return e;
}
