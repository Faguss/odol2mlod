#pragma once
#include <cstddef>
#include <iterator>
#include "type_traits.hpp"
#include "utility.hpp"
#include <array>

namespace fp {
template <class T, size_t sizeV = static_cast<size_t>(-1)>
class span;

namespace internal {
namespace span {
template <class T, class SpanElementType,
	bool result = std::is_same<size_t, decltype(reinterpret_cast<const T*>(0)->size())>::value&&
		std::is_convertible<typename std::remove_pointer<decltype(reinterpret_cast<T*>(0)->data())>::type (*)[],
			SpanElementType (*)[]>::value>
constexpr bool is_span_compatible_helper(size_t) {
	return result;
}

template <class T, class SpanElementType>
constexpr bool is_span_compatible_helper(...) {
	return false;
}

template <class T, class SpanElementType,
	bool result = std::is_same<size_t, decltype(reinterpret_cast<const T*>(0)->size())>::value&&
		std::is_convertible<typename std::remove_pointer<decltype(reinterpret_cast<const T*>(0)->data())>::type (*)[],
			SpanElementType (*)[]>::value>
constexpr bool is_const_type_span_compatible_helper(size_t) {
	return result;
}

template <class T, class SpanElementType>
constexpr bool is_const_type_span_compatible_helper(...) {
	return false;
}
template <class T, class F>
T cast(F from) {
	return from;
}
} // namespace span
} // namespace internal

template <class T, class SpanElementType>
constexpr bool is_span_compatible() {
	return internal::span::is_span_compatible_helper<T, SpanElementType>(0);
}
template <class T, class SpanElementType>
constexpr bool is_const_type_span_compatible() {
	return internal::span::is_const_type_span_compatible_helper<T, SpanElementType>(0);
}

/** \brief implementation of std::span
 * <a href="https://en.cppreference.com/w/cpp/container/span">https://en.cppreference.com/w/cpp/container/span</a>
 */
template <class T, size_t sizeV>
class span {
public:
	using element_type = T;
	using value_type = std::remove_cv_t<T>;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = T*;
	using const_iterator = const T*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using size_type = size_t;
	using difference_type = ptrdiff_t;

	constexpr span() noexcept : m_data(nullptr) {
		static_assert(sizeV > 0, "Error default ctor for empty staticly sized span called");
	}
	template <size_t N>
	constexpr span(element_type (&arr)[N]) noexcept : m_data(arr) {
		static_assert(N == sizeV, "Error static sized span from carray size mismatch");
	}
	constexpr span(pointer ptr) noexcept : m_data(ptr) {}
	constexpr span(std::array<typename std::remove_cv<T>::type, static_cast<size_t>(sizeV)>& array) noexcept
		: m_data(array.data()) {}

	// compile time array span
	template <size_t arraySize>
	constexpr span(const std::array<typename std::remove_cv<T>::type, arraySize>& array,
		typename std::enable_if<std::is_const<T>::value && arraySize == static_cast<size_t>(sizeV)>::type* = 0) noexcept
		: m_data(array.data()) {}
	template <size_t arraySize>
	constexpr span(const std::array<T, arraySize>& array,
		typename std::enable_if<arraySize == static_cast<size_t>(sizeV) && std::is_const<T>::value>::type* = 0) noexcept
		: m_data(array.data()) {}

	// size
	constexpr size_type size() const noexcept { return sizeV; }
	constexpr size_type size_bytes() const noexcept { return sizeV * sizeof(T); }
	constexpr bool empty() const noexcept { return (size() == 0); }

	// data access
	constexpr pointer data() const noexcept { return m_data; }
	constexpr reference operator[](size_type i) const noexcept {
		FP_STD_DEBUG_ASSERT(size() > i, "index is out of range")
		return m_data[i];
	}

	// iterators
	constexpr iterator begin() const noexcept { return data(); }
	constexpr const_iterator cbegin() const noexcept { return data(); }
	constexpr iterator end() const noexcept { return data() + size(); }
	constexpr const_iterator cend() const noexcept { return data() + size(); }

	constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
	constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
	constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
	constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

	// front, back
	constexpr T& front() const noexcept {
		FP_STD_DEBUG_ASSERT(!empty(), "front called on empty span")
		return *data();
	}
	constexpr T& back() const noexcept {
		FP_STD_DEBUG_ASSERT(!empty(), "back called on empty span")
		return *(data() + size() - 1);
	}

	// sub spans
	constexpr auto first(size_t count) const noexcept {
		FP_STD_DEBUG_ASSERT(count <= size(), "number of first elemnts is out of range")
		return span<T, static_cast<size_t>(-1)>(m_data, count);
	}
	constexpr auto last(size_t count) const noexcept {
		FP_STD_DEBUG_ASSERT(count <= size(), "number of last elemnts is out of range")
		return span<T, static_cast<size_t>(-1)>(m_data + (sizeV - count), count);
	}
	constexpr auto subspan(size_t offset) const noexcept {
		FP_STD_DEBUG_ASSERT(offset <= size(), "offset is out of range")
		return span<T, static_cast<size_t>(-1)>(m_data + offset, sizeV - offset);
	}
	constexpr auto subspan(size_t offset, size_t count) const noexcept {
		FP_STD_DEBUG_ASSERT(offset + count <= size(), "offset + count is out of range")
		return span<T, static_cast<size_t>(-1)>(m_data + offset, count);
	}

	// sub spans
	template <size_t count>
	constexpr auto first() const noexcept {
		static_assert(count <= sizeV, "number of first elemnts is out of range");
		return span<T, count>(m_data);
	}
	template <size_t count>
	constexpr auto last() const noexcept {
		static_assert(count <= sizeV, "number of last elemnts is out of range");
		return span<T, count>(m_data + (sizeV - count));
	}
	template <size_t offset>
	constexpr auto subspan() const noexcept {
		static_assert(offset <= sizeV, "offset is out of range");
		return span<T, sizeV - offset>(m_data + offset);
	}
	template <size_t offset, size_t count>
	constexpr auto subspan() const noexcept {
		static_assert(offset + count <= sizeV, "offset + count is out of range");
		return span<T, count>(m_data + offset);
	}

private:
	T* m_data;
};

/** \private */
template <class T>
class span<T, static_cast<size_t>(-1)> {
public:
	using element_type = T;
	using value_type = std::remove_cv_t<T>;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = T*;
	using const_iterator = const T*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using size_type = size_t;
	using difference_type = ptrdiff_t;

	template <size_t N>
	constexpr span(element_type (&arr)[N]) noexcept : m_data(arr), m_size(N) {}
	constexpr span() noexcept : m_data(nullptr), m_size(0) {}

	// note &(*first) is important because first can be iterator to linear data
	template <class It>
	constexpr span(It firstIt, size_type count) noexcept : m_data(&(*firstIt)), m_size(count) {}

	// note &(*firstIt) is important because firstIt can be iterator to linear data
	template <class It, class End,
		class = typename std::enable_if<(sizeof(static_cast<size_t>(std::declval<It>() - std::declval<End>())) > 0u &&
										 sizeof(std::declval<It>() != std::declval<End>()) > 0u)>::type>
	constexpr span(It firstIt, End lastIt) noexcept
		: m_data(&(*firstIt)), m_size(static_cast<size_t>(lastIt - firstIt)) {}

	template <class S>
	constexpr span(S& spanCompatible, typename std::enable_if<is_span_compatible<S, T>()>::type* = 0) noexcept
		: m_data(internal::span::cast<pointer>(spanCompatible.data())), m_size(spanCompatible.size()) {}

	template <class S>
	constexpr span(
		const S& spanCompatible, typename std::enable_if<is_const_type_span_compatible<S, T>()>::type* = 0) noexcept
		: m_data(internal::span::cast<pointer>(spanCompatible.data())), m_size(spanCompatible.size()) {}

	// size
	constexpr size_type size() const noexcept { return m_size; }
	constexpr size_type size_bytes() const noexcept { return size() * sizeof(T); }
	constexpr bool empty() const noexcept { return (size() == 0); }

	// data access
	constexpr pointer data() const noexcept { return m_data; }
	constexpr reference operator[](size_type i) const noexcept {
		FP_STD_DEBUG_ASSERT(size() > i, "index is out of range")
		return m_data[i];
	}

	// iterators
	constexpr iterator begin() const noexcept { return data(); }
	constexpr const_iterator cbegin() const noexcept { return data(); }
	constexpr iterator end() const noexcept { return data() + size(); }
	constexpr const_iterator cend() const noexcept { return data() + size(); }

	constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
	constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
	constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
	constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

	// front, back
	constexpr T& front() const noexcept {
		FP_STD_DEBUG_ASSERT(!empty(), "front called on empty span")
		return *data();
	}
	constexpr T& back() const noexcept {
		FP_STD_DEBUG_ASSERT(!empty(), "back called on empty span")
		return *(data() + size() - 1);
	}
	// sub spans
	constexpr span first(size_t count) const noexcept {
		FP_STD_DEBUG_ASSERT(count <= size(), "number of first elemnts is out of range")
		return span(m_data, count);
	}
	constexpr span last(size_t count) const noexcept {
		FP_STD_DEBUG_ASSERT(count <= size(), "number of last elemnts is out of range")
		return span(m_data + (m_size - count), count);
	}
	constexpr span subspan(size_t offset) const noexcept {
		FP_STD_DEBUG_ASSERT(offset <= size(), "offset is out of range")
		return span(m_data + offset, m_size - offset);
	}
	constexpr span subspan(size_t offset, size_t count) const noexcept {
		FP_STD_DEBUG_ASSERT(offset + count <= size(), "offset + count is out of range")
		return span(m_data + offset, count);
	}

	// sub spans
	template <size_t count>
	constexpr auto first() const noexcept {
		FP_STD_DEBUG_ASSERT(count <= size(), "number of first elemnts is out of range")
		return span<T, count>(m_data);
	}
	template <size_t count>
	constexpr auto last() const noexcept {
		FP_STD_DEBUG_ASSERT(count <= size(), "number of last elemnts is out of range")
		return span<T, count>(m_data + (size() - count));
	}
	template <size_t offset>
	constexpr auto subspan() const noexcept {
		FP_STD_DEBUG_ASSERT(offset <= size(), "offset is out of range")
		return span<T>(m_data + offset, size() - offset);
	}
	template <size_t offset, size_t count>
	constexpr auto subspan() const noexcept {
		FP_STD_DEBUG_ASSERT(offset + count <= size(), "offset + count is out of range")
		return span<T, count>(m_data + offset);
	}

private:
	T* m_data;
	size_type m_size;
};

// type deduction guidelines
/** \private */
template <class It>
span(It first, size_t size)
	-> span<typename std::remove_pointer<typename std::iterator_traits<It>::pointer>::type, static_cast<size_t>(-1)>;

/** \private */
template <class It, class End>
span(It first, End last)
	-> span<typename std::remove_pointer<typename std::iterator_traits<It>::pointer>::type, static_cast<size_t>(-1)>;

/** \private */
template <class S>
span(S& spanCompatible)
	-> span<typename std::remove_pointer<decltype(reinterpret_cast<S*>(0)->data())>::type, static_cast<size_t>(-1)>;

/** \private */
template <class S>
span(const S& spanCompatible)
	-> span<typename std::remove_pointer<decltype(reinterpret_cast<const S*>(0)->data())>::type,
		static_cast<size_t>(-1)>;

/** \private */
template <class T, size_t N>
span(T (&)[N]) -> span<T, N>;

/** \private */
template <class T, size_t N>
span(std::array<T, N>&) -> span<T, N>;

/** \private */
template <class T, size_t N>
span(const std::array<T, N>&) -> span<const T, N>;

// span algorithms
template <class T, size_t N>
constexpr inline span<std::byte> as_writable_bytes(span<T, N> s) noexcept {
	return span<std::byte>(reinterpret_cast<std::byte*>(s.data()), s.size_bytes());
}
template <class T, size_t N>
constexpr inline span<const std::byte> as_bytes(span<T, N> s) noexcept {
	return span<const std::byte>(reinterpret_cast<const std::byte*>(s.data()), s.size_bytes());
}

template <class T>
inline fp::span<std::byte> to_writable_bytes(T&& t) noexcept {
	using decayT = std::decay_t<T>;
	static_assert(!std::is_const_v<decayT>, "Can not write into const objects.");
	static_assert(
		std::is_trivially_copyable_v<decayT>, "Raw serialization of objects with non default copy is not allowed");
	return fp::span(reinterpret_cast<std::byte*>(&t), sizeof(decayT));
}

template <class T>
inline fp::span<const std::byte> to_bytes(const T& t) noexcept {
	static_assert(
		std::is_trivially_copyable<T>::value, "Raw serialization of objects with non default copy is not allowed");
	return fp::span(reinterpret_cast<const std::byte*>(&t), sizeof(t));
}

template <class T, class U>
constexpr inline auto span_copy(
	T&& from, U&& to, std::enable_if_t<is_linear_range<std::decay_t<T>>() && is_linear_range<std::decay_t<U>>()>* = 0) {
	static_assert(!std::is_const_v<typename std::remove_pointer_t<typename std::decay_t<U>::pointer>>,
		"Can not copy into const objects.");
	const auto size = std::min(from.size(), to.size());
	std::copy(from.data(), from.data() + size, to.data());
	return span(to.data(), size);
}

template <class T, class U>
constexpr inline auto span_move(
	T&& from, U&& to, std::enable_if_t<is_linear_range<std::decay_t<T>>() && is_linear_range<std::decay_t<U>>()>* = 0) {
	static_assert(!std::is_const_v<typename std::remove_pointer_t<typename std::decay_t<U>::pointer>>,
		"Can not move into const objects.");
	const auto size = std::min(from.size(), to.size());
	std::move(from.data(), from.data() + to.size(), to.data());
	return span(to.data(), size);
}

} // namespace fp

