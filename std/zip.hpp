#pragma once
#include <tuple>
#include <type_traits>
#include <iterator>
namespace fp {
namespace internal {
namespace zip {
template <class Range>
inline constexpr size_t min_size_2(size_t previousSize, Range&& range) noexcept {
	return std::min(previousSize, range.size());
}

template <class Range, class... Ranges>
inline constexpr size_t min_size_2(size_t previousSize, Range&& range, Ranges&&... ranges) noexcept {
	return min_size_2(std::min(previousSize, range.size()), std::forward<Ranges>(ranges)...);
}

template <class Range, class... Ranges>
inline constexpr size_t min_size(Range&& range, Ranges&&... ranges) noexcept {
	return min_size_2(range.size(), std::forward<Ranges>(ranges)...);
}

template <class T, bool result = (sizeof(decltype(reinterpret_cast<const T*>(0)->size())) > 0) &&
								 (sizeof(decltype(reinterpret_cast<T*>(0)->data())) > 0)>
inline constexpr bool is_linear_range(size_t) {
	return result;
}

template <class T>
inline constexpr bool is_linear_range(...) {
	return false;
}
template <class T>
using interator_t = std::conditional_t<is_linear_range<std::decay_t<T>>(0),
	std::conditional_t<std::is_const_v<std::remove_reference_t<T>>, typename std::decay_t<T>::const_pointer,
		typename std::decay_t<T>::pointer>,
	std::conditional_t<std::is_const_v<std::remove_reference_t<T>>, typename std::decay_t<T>::const_iterator,
		typename std::decay_t<T>::iterator>>;

template <class T>
inline constexpr auto get_begin_iterator(T&& container) noexcept {
	if constexpr (is_linear_range<std::decay_t<T>>(0)) {
		return container.data();
	} else {
		return container.begin();
	}
}

template <class T>
inline constexpr auto get_end_iterator(T&& container) noexcept {
	if constexpr (is_linear_range<std::decay_t<T>>(0)) {
		return container.data(); // still data because of m_index redeferencing
	} else {
		return container.end();
	}
}

template <class T>
inline constexpr void increase_non_pointer_iterator(T& it) noexcept {
	if constexpr (!std::is_pointer_v<std::remove_reference_t<T>>) {
		++it;
	}
}

template <class T>
inline constexpr void decrease_non_pointer_iterator(T& it) noexcept {
	if constexpr (!std::is_pointer_v<std::remove_reference_t<T>>) {
		--it;
	}
}

} // namespace zip
} // namespace internal




/// zip iterator
template <bool hasIndex, class... Iterators>
class zip_iterator : public std::iterator<std::bidirectional_iterator_tag, zip_iterator<hasIndex, Iterators...>> {
public:
	using base = std::iterator<std::bidirectional_iterator_tag, zip_iterator<hasIndex, Iterators...>, std::ptrdiff_t>;

	constexpr zip_iterator(const zip_iterator& other)  noexcept : m_iters(other.m_iters), m_index(other.m_index) {}
	constexpr zip_iterator& operator=(const zip_iterator& other) noexcept { m_iters = other.m_iters; m_index = other.m_index; }

	constexpr zip_iterator(Iterators... iters) noexcept : m_iters(iters...), m_index(0) {}

	template <class... Data>
	constexpr zip_iterator(size_t index, Iterators... iters) noexcept : m_iters(iters...), m_index(index) {}


	template <size_t index>
	constexpr typename std::iterator_traits<std::remove_reference_t<decltype(std::get<index>(std::declval<std::tuple<Iterators..., const size_t*>&>()))>>::reference get() noexcept {
		if constexpr (index == sizeof...(Iterators)) {
			return m_index;
		} else {
			using T = std::remove_reference_t<decltype(std::get<index>(m_iters))>;
			if constexpr (std::is_pointer_v<T>) {
				return std::get<index>(m_iters)[m_index];
			} else {
				return *std::get<index>(m_iters);
			}
		}
	}

	// iterator stuff
	constexpr const typename base::reference operator*() const noexcept { return *this; }
	constexpr typename base::reference operator*() noexcept { return *this; }

	constexpr zip_iterator& operator++() noexcept {
		++m_index;
		increase_non_pointer_iterators(std::make_index_sequence<sizeof...(Iterators)>());
		return *this;
	}

	constexpr zip_iterator& operator--() noexcept {
		--m_index;
		decrease_non_pointer_iterators(std::make_index_sequence<sizeof...(Iterators)>());
		return *this;
	}

	constexpr bool operator==(const zip_iterator& other) const noexcept { return m_index == other.m_index; }
	constexpr bool operator!=(const zip_iterator& other) const noexcept { return m_index != other.m_index; }

	constexpr bool operator<(const zip_iterator& other) const noexcept { return m_index < other.m_index; }
	constexpr bool operator>(const zip_iterator& other) const noexcept { return m_index > other.m_index; }

	constexpr bool operator<=(const zip_iterator& other) const noexcept { return m_index <= other.m_index; }
	constexpr bool operator>=(const zip_iterator& other) const noexcept { return m_index >= other.m_index; }

private:
	template <std::size_t... Is>
	constexpr void increase_non_pointer_iterators(std::index_sequence<Is...>) noexcept {
		(internal::zip::increase_non_pointer_iterator(std::get<Is>(m_iters)), ...);
	}
	template <std::size_t... Is>
	constexpr void decrease_non_pointer_iterators(std::index_sequence<Is...>) noexcept {
		(internal::zip::decrease_non_pointer_iterator(std::get<Is>(m_iters)), ...);
	}

	std::tuple<Iterators...> m_iters;
	size_t m_index;
};

/// zip iterator is stashing iterator therefore it can not use std::reverse_iterator
template <class Iterator>
class reverse_zip_iterator : public std::iterator<
	typename std::iterator_traits<Iterator>::iterator_category,
	typename std::iterator_traits<Iterator>::value_type,
	typename std::iterator_traits<Iterator>::difference_type,
	typename std::iterator_traits<Iterator>::pointer,
	typename std::iterator_traits<Iterator>::reference> {
public:
	using base = std::iterator<
		typename std::iterator_traits<Iterator>::iterator_category,
		typename std::iterator_traits<Iterator>::value_type,
		typename std::iterator_traits<Iterator>::difference_type,
		typename std::iterator_traits<Iterator>::pointer,
		typename std::iterator_traits<Iterator>::reference>;

	constexpr reverse_zip_iterator(Iterator iter) noexcept : m_iter(--iter) {}

	// iterator stuff
	constexpr const typename base::reference operator*() const noexcept { return *m_iter; }
	constexpr typename base::reference operator*() noexcept { return *m_iter; }

	constexpr reverse_zip_iterator& operator++() noexcept {
		--m_iter;
		return *this;
	}

	constexpr reverse_zip_iterator& operator--() noexcept {
		++m_iter;
	}

	constexpr bool operator==(const reverse_zip_iterator& other) const noexcept { return m_iter == other.m_iter; }
	constexpr bool operator!=(const reverse_zip_iterator& other) const noexcept { return m_iter != other.m_iter; }

	constexpr bool operator<(const reverse_zip_iterator& other) const noexcept { return m_iter < other.m_iter; }
	constexpr bool operator>(const reverse_zip_iterator& other) const noexcept { return m_iter > other.m_iter; }

	constexpr bool operator<=(const reverse_zip_iterator& other) const noexcept { return m_iter <= other.m_iter; }
	constexpr bool operator>=(const reverse_zip_iterator& other) const noexcept { return m_iter >= other.m_iter; }

private:
	Iterator m_iter;
};

/** \brief Range with fp::zip_iterator.
 * Intended for use in range for.
 */
template <class... Ranges>
class zip {
	static_assert(sizeof...(Ranges) > 1, "come on don't use zip when you dont have at least 2 ranges");

public:
	constexpr zip(Ranges&&... ranges)
		: m_begin(internal::zip::get_begin_iterator(ranges)...),
		  m_end(internal::zip::min_size(std::forward<Ranges>(ranges)...), internal::zip::get_end_iterator(ranges)...) {}

	constexpr auto begin() noexcept { return m_begin; }
	constexpr auto end() noexcept { return m_end; }
	constexpr auto begin() const noexcept { return m_begin; }
	constexpr auto end() const noexcept { return m_end; }
	constexpr auto cbegin() const noexcept { return m_begin; }
	constexpr auto cend() const noexcept { return m_end; }

	constexpr auto rbegin() noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto rend() noexcept { return reverse_zip_iterator(m_begin); }
	constexpr auto rbegin() const noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto rend() const noexcept { return reverse_zip_iterator(m_begin); }
	constexpr auto crbegin() const noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto crend() const noexcept { return reverse_zip_iterator(m_begin); }

private:
	zip_iterator<false, internal::zip::interator_t<Ranges>...> m_begin;
	zip_iterator<false, internal::zip::interator_t<Ranges>...> m_end;
};
/** \brief Range with fp::zip_iterator with element index.
 * Intended for use in range for.
 */
template <class... Ranges>
class zip_index {
	static_assert(sizeof...(Ranges) > 1, "come on don't use zip_index when you dont have at least 2 ranges");

public:
	constexpr zip_index(Ranges&&... ranges)
		: m_begin(internal::zip::get_begin_iterator(ranges)...),
		  m_end(internal::zip::min_size(std::forward<Ranges>(ranges)...), internal::zip::get_end_iterator(ranges)...) {}

	constexpr auto begin() noexcept { return m_begin; }
	constexpr auto end() noexcept { return m_end; }
	constexpr auto begin() const noexcept { return m_begin; }
	constexpr auto end() const noexcept { return m_end; }
	constexpr auto cbegin() const noexcept { return m_begin; }
	constexpr auto cend() const noexcept { return m_end; }

	constexpr auto rbegin() noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto rend() noexcept { return reverse_zip_iterator(m_begin); }
	constexpr auto rbegin() const noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto rend() const noexcept { return reverse_zip_iterator(m_begin); }
	constexpr auto crbegin() const noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto crend() const noexcept { return reverse_zip_iterator(m_begin); }

private:
	zip_iterator<true, internal::zip::interator_t<Ranges>...> m_begin;
	zip_iterator<true, internal::zip::interator_t<Ranges>...> m_end;
};

template <class Range>
class enumerate {
public:
	constexpr enumerate(Range&& range)
		: m_begin(internal::zip::get_begin_iterator(range)), m_end(range.size(), internal::zip::get_end_iterator(range)) {}

	constexpr auto begin() noexcept { return m_begin; }
	constexpr auto end() noexcept { return m_end; }
	constexpr auto begin() const noexcept { return m_begin; }
	constexpr auto end() const noexcept { return m_end; }
	constexpr auto cbegin() const noexcept { return m_begin; }
	constexpr auto cend() const noexcept { return m_end; }

	constexpr auto rbegin() noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto rend() noexcept { return reverse_zip_iterator(m_begin); }
	constexpr auto rbegin() const noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto rend() const noexcept { return reverse_zip_iterator(m_begin); }
	constexpr auto crbegin() const noexcept { return reverse_zip_iterator(m_end); }
	constexpr auto crend() const noexcept { return reverse_zip_iterator(m_begin); }
private:
	zip_iterator<true, internal::zip::interator_t<Range>> m_begin;
	zip_iterator<true, internal::zip::interator_t<Range>> m_end;
};

// type deduction guidelines
template <class F, class... R>
zip(F&&, R&&...)->zip<F, R...>;

template <class F, class... R>
zip_index(F&&, R&&...)->zip_index<F, R...>;

template <class T>
enumerate(T &&)->enumerate<T>;
} // namespace fp

namespace std {
template <class... Types>
class tuple_size<fp::zip_iterator<false, Types...>> : public std::integral_constant<std::size_t, sizeof...(Types)> {};

template <class... Types>
class tuple_size<fp::zip_iterator<true, Types...>> : public std::integral_constant<std::size_t, sizeof...(Types) + 1> {
};

template <size_t index, class... Types>
struct tuple_element<index, fp::zip_iterator<false, Types...>> {
	using type = decltype(std::declval<fp::zip_iterator<false, Types...>&>().template get<index>());
};
template <size_t index, class... Types>
struct tuple_element<index, fp::zip_iterator<true, Types...>> {
	using type = decltype(std::declval<fp::zip_iterator<true, Types...>&>().template get<index>());
};
} // namespace std

