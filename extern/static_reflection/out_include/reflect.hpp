#ifndef REFLECT_H
#define REFLECT_H

#include <tuple>
#include <type_traits>
#include <utility>

namespace reflect {
template <typename T>
struct Reflect {};

enum class ObjectType { CLASS, STRUCT, UNION, ENUM };

template <class T>
struct is_public {
  static constexpr bool value = true;
};

template <class T>
struct is_protected {
  static constexpr bool value = true;
};

template <class T>
struct is_private {
  static constexpr bool value = true;
};

template <class T>
constexpr auto is_public_v = is_public<T>::value;
template <class T>
constexpr auto is_protected_v = is_protected<T>::value;
template <class T>
constexpr auto is_private_v = is_private<T>::value;

template <class T1, class T2>
struct reflects_same;
template <class T>
struct get_source_line;
template <class T>
struct get_source_column;
template <class T>
struct get_source_file_name;
template <class T1, class T2>
constexpr auto reflects_same_v = reflects_same<T1, T2>::value;
template <class T>
constexpr auto get_source_line_v = get_source_line<T>::value;
template <class T>
constexpr auto get_source_column_v = get_source_column<T>::value;
template <class T>
constexpr auto get_source_file_name_v = get_source_file_name<T>::value;

// for not we use tupple instead od sequence
template <typename T>
constexpr auto get_size_v = std::tuple_size<typename T::type>();

template <int N, typename T>
using get_element_t = std::tuple_element_t<N, typename T::type>;

// 21.11.4.4 Named operations
template <class T>
struct is_unnamed;
template <class T>
struct get_name {
  static constexpr auto value = T::name;
};
template <class T>
struct get_display_name;

template <class T>
constexpr auto is_unnamed_v = is_unnamed<T>::value;
template <class T>
constexpr auto get_name_v = get_name<T>::value;
template <class T>
constexpr auto get_display_name_v = get_display_name<T>::value;

// 21.11.4.5 Alias operations
template <class T>
struct get_aliased;

template <class T>
using get_aliased_t = typename get_aliased<T>::type;

// 21.11.4.6 Type operations
template <class T>
struct get_type {
  using type = typename T::type;
};
template <class T>
struct get_reflected_type {
  // TODO: change to reflected type
  using type = typename T::type;
};

template <class T>
struct is_enum {
  static constexpr bool value = T::object_type == ObjectType::ENUM;
};
template <class T>
struct is_class {
  static constexpr bool value = T::object_type == ObjectType::CLASS;
};
template <class T>
struct is_struct {
  static constexpr bool value = T::object_type == ObjectType::STRUCT;
};
template <class T>
struct is_union {
  static constexpr bool value = T::object_type == ObjectType::UNION;
};

template <class T>
using get_type_t = typename get_type<T>::type;
template <class T>
using get_reflected_type_t = typename get_reflected_type<T>::type;

template <class T>
constexpr auto is_enum_v = is_enum<T>::value;
template <class T>
constexpr auto is_class_v = is_class<T>::value;
template <class T>
constexpr auto is_struct_v = is_struct<T>::value;
template <class T>
constexpr auto is_union_v = is_union<T>::value;

// helper for now
namespace helper {
template <typename T, int N>
struct PublicMember {
  static constexpr auto pointer = std::get<N>(T::public_data_members);
  using type = std::tuple_element_t<N, typename T::public_data_member_types>;
};

template <template <typename, int> class M, typename T, typename U>
struct selector;

template <template <typename, int> class M, typename T, std::size_t... Is>
struct selector<M, T, std::index_sequence<Is...>> {
  using type = std::tuple<M<T, Is>...>;
};
}  // namespace helper

// 21.11.4.8 Record operations
template <class T>
struct get_public_data_members {
  static constexpr int N = std::tuple_size<decltype(T::public_data_members)>();
  using type = typename helper::selector<helper::PublicMember, T,
                                         std::make_index_sequence<N>>::type;
};
template <class T>
struct get_accessible_data_members;
template <class T>
struct get_data_members;
template <class T>
struct get_public_member_types;
template <class T>
struct get_accessible_member_types;
template <class T>
struct get_member_types;
template <class T>
struct get_public_base_classes {
  using type = typename T::public_base_classes;
};
template <class T>
struct get_accessible_base_classes;
template <class T>
struct get_base_classes;
template <class T>
struct is_final;

template <class T>
using get_public_data_members_t = typename get_public_data_members<T>::type;
template <class T>
using get_accessible_data_members_t =
    typename get_accessible_data_members<T>::type;
template <class T>
using get_data_members_t = typename get_data_members<T>::type;
template <class T>
using get_public_member_types_t = typename get_public_member_types<T>::type;
template <class T>
using get_accessible_member_types_t =
    typename get_accessible_member_types<T>::type;
template <class T>
using get_member_types_t = typename get_member_types<T>::type;
template <class T>
using get_public_base_classes_t = typename get_public_base_classes<T>::type;
template <class T>
using get_accessible_base_classes_t =
    typename get_accessible_base_classes<T>::type;
template <class T>
using get_base_classes_t = typename get_base_classes<T>::type;
template <class T>
constexpr auto is_final_v = is_final<T>::value;

// 21.11.4.9 Enum operations
template <class T>
struct is_scoped_enum;
template <class T>
struct get_enumerators;
template <class T>
struct get_underlying_type;

template <class T>
constexpr auto is_scoped_enum_v = is_scoped_enum<T>::value;
template <class T>
using get_enumerators_t = typename get_enumerators<T>::type;
template <class T>
using get_underlying_type_t = typename get_underlying_type<T>::type;

// 21.11.4.10 Value operations
template <class T>
struct get_constant;
template <class T>
struct is_constexpr;
template <class T>
struct is_static;
template <class T>
struct get_pointer {
  static constexpr auto value = T::pointer;
};

template <class T>
constexpr auto get_constant_v = get_constant<T>::value;
template <class T>
constexpr auto is_constexpr_v = is_constexpr<T>::value;
template <class T>
constexpr auto is_static_v = is_static<T>::value;
template <class T>
const auto get_pointer_v = get_pointer<T>::value;

// 21.11.4.11 Base operations
template <class T>
struct get_class;
template <class T>
struct is_virtual;

template <class T>
using get_class_t = typename get_class<T>::type;
template <class T>
constexpr auto is_virtual_v = is_virtual<T>::value;

// 21.11.4.12 Namespace operations
template <class T>
struct is_inline;

template <class T>
constexpr auto is_inline_v = is_inline<T>::value;
}  // namespace reflect

template <typename T>
using reflexpr = reflect::Reflect<T>;

#endif  // REFLECT_H
