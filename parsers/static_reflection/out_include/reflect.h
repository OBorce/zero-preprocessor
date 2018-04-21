#ifndef REFLECT_H
#define REFLECT_H

#include <tuple>

namespace reflect {
template <typename T>
struct Reflect {};

//TODO: add reflection stuff here

template<typename T>
constexpr auto get_name = T::name;

}  // namespace reflect

template<typename T>
using reflexpr = reflect::Reflect<T>;

#endif  //! REFLECT_H
