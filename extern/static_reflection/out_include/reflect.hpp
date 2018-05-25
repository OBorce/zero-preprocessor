#ifndef REFLECT_H
#define REFLECT_H

#include <tuple>

namespace reflect {
template <typename T>
struct Reflect {};

template <typename T, int N>
struct Member {
  static constexpr auto pointer = std::get<N>(T::members);
};

template <typename T, typename U>
struct selector;

template <typename T, std::size_t... Is>
struct selector<T, std::index_sequence<Is...>> {
  using type = std::tuple<Member<T, Is>...>;
};

template <typename T>
struct make_members {
  static constexpr int N = std::tuple_size<decltype(T::members)>();
  using type = typename selector<T, std::make_index_sequence<N>>::type;
};

// TODO: add reflection stuff here

template <typename T>
constexpr auto get_name = T::name;

template <typename T>
using get_public_members = typename make_members<T>::type;

template <typename T>
constexpr auto get_pointer_v = T::pointer;

}  // namespace reflect

template <typename T>
using reflexpr = reflect::Reflect<T>;

#endif  // REFLECT_H
