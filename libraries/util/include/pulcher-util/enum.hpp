#pragma once

#include <cstdint>
#include <type_traits>

// -- useful functions to eliminate need of static_cast

template <typename EnumType> constexpr auto Idx(EnumType const & v) {
  return static_cast<typename std::underlying_type<EnumType>::type>(v);
}

template <typename EnumType>
constexpr typename std::underlying_type<EnumType>::type & Idx(EnumType & v) {
  return reinterpret_cast<typename std::underlying_type<EnumType>::type &>(v);
}
