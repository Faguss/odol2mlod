#pragma once
#include <type_traits>

namespace fp {
namespace internal {
namespace type_traits {
template <class Object, class... Args>
constexpr inline bool has_constructor(int, std::enable_if_t<(sizeof(Object(std::declval<Args>()...)) > 0u)>* = 0) {
	return true;
}

template <class Object, class... Args>
constexpr inline bool has_constructor(...) {
	return false;
}

template <class T,
	bool result = (sizeof(typename T::value_type) > 0 && sizeof(decltype(std::declval<T&>().data())) > 0 &&
				   sizeof(decltype(std::declval<T&>().size())) > 0)>
constexpr bool is_linear_range_helper(int) {
	return result;
}

template <class T>
constexpr bool is_linear_range_helper(...) {
	return false;
}
} // namespace type_traits
} // namespace internal

template <class Object, class... Args>
constexpr inline bool has_constructor(...) {
	return internal::type_traits::has_constructor<Object, Args...>(0);
}

template <class T>
constexpr bool is_linear_range() {
	return internal::type_traits::is_linear_range_helper<T>(0);
}
} // namespace fp

