#pragma once
#include <cstdarg>
#include <cstdio>
#include "span.hpp"
#include "type_traits.hpp"
namespace fp {
/** \brief Wrapper for FILE
 */
class file {
public:
	file() noexcept : m_file(nullptr) {}
	file(const char* name, const char* flags) noexcept : m_file(std::fopen(name, flags)) {}
	file(const std::string& name, const char* flags) noexcept : m_file(std::fopen(name.c_str(), flags)) {}

	~file() noexcept { close_internal(); }

	// no copy
	file(const file&) = delete;
	file& operator=(const file&) = delete;

	// move operations
	file(file&& other) noexcept : m_file(other.m_file) { other.m_file = nullptr; }

	file& operator=(file&& other) noexcept {
		close_internal();
		m_file = other.m_file;
		other.m_file = nullptr;

		return *this;
	}

	void swap(file& other) noexcept { std::swap(m_file, other.m_file); }

	void open(const char* name, const char* flags) noexcept {
		close_internal();
		m_file = std::fopen(name, flags);
	}
	void open(const std::string& name, const char* flags) noexcept {
		close_internal();
		m_file = std::fopen(name.c_str(), flags);
	}

	void close() noexcept {
		close_internal();
		m_file = nullptr;
	}

	FILE* release() noexcept {
		FILE* temp = m_file;
		m_file = nullptr;
		return temp;
	}

	explicit operator bool() const noexcept { return (m_file); }
	bool is_open() const noexcept { return m_file; }

	int flush() noexcept {
		if (!m_file) {
			return EOF;
		}
		return std::fflush(m_file);
	}

	template <class T>
	size_t read(T&& container, std::enable_if_t<is_linear_range<std::decay_t<T>>()>* = 0) noexcept {
		using U = std::decay_t<T>;
		static_assert(!std::is_const<typename U::value_type>::value, "Can not write into const objects.");
		static_assert(std::is_trivially_destructible<typename U::value_type>::value,
			"Raw serialization of objects with non default destructors is not allowed.");
		if (!m_file) {
			return 0;
		}
		return std::fread(container.data(), 1, container.size() * sizeof(typename U::value_type), m_file);
	}

	template <class T>
	size_t write(const T& container, std::enable_if_t<is_linear_range<std::decay_t<T>>()>* = 0) noexcept {
		using U = std::decay_t<T>;
		static_assert(std::is_trivially_destructible<typename U::value_type>::value,
			"Raw serialization of objects with non default destructors is not allowed.");
		if (!m_file) {
			return 0;
		}
		return std::fwrite(container.data(), 1, container.size() * sizeof(typename U::value_type), m_file);
	}

	size_t read(void* data, size_t size) noexcept {
		if (!m_file) {
			return 0;
		}
		return std::fread(data, 1, size, m_file);
	}
	size_t write(const void* data, size_t size) noexcept {
		if (!m_file) {
			return 0;
		}
		return std::fwrite(data, 1, size, m_file);
	}

	template <class T>
	size_t read_all(T&& container, size_t offset = 0) {
		using U = std::decay_t<T>;
		// static_assert(is_resizable_container<U>(), "read_all requires resizable container");
		static_assert(sizeof(typename U::value_type) == 1, "read_all requires container with byte sized type");
		static_assert(!std::is_const<typename U::value_type>::value, "Can not write into const objects.");
		static_assert(std::is_trivially_destructible<typename U::value_type>::value,
			"Raw serialization of objects with non default destructors is not allowed.");
		if (!m_file) {
			container.resize(offset);
			return 0;
		}
		const auto cur = ftell(m_file);
		if (std::fseek(m_file, 0, SEEK_END) != 0) {
			container.resize(offset);
			return 0;
		}
		const auto end = ftell(m_file);
		if (std::fseek(m_file, cur, SEEK_SET) != 0) {
			container.resize(offset);
			return 0;
		}
		if (cur >= end) {
			container.resize(offset);
			return 0;
		}

		const size_t readSize = static_cast<size_t>(end - cur);
		container.resize(offset + readSize);
		auto ret = std::fread(container.data() + offset, 1, readSize, m_file);
		container.resize(offset + ret);
		return ret;
	}

	void rewind() noexcept {
		if (!m_file) {
			return;
		}
		std::rewind(m_file);
	}
	bool eof() noexcept {
		if (!m_file) {
			return true;
		}
		return (std::feof(m_file) != 0);
	}

	bool error() noexcept {
		if (!m_file) {
			return false;
		}
		return (std::ferror(m_file) != 0);
	}
	void clear_error() noexcept {
		if (!m_file) {
			return;
		}
		std::clearerr(m_file);
	}

	int getc() noexcept {
		if (!m_file) {
			return EOF;
		}
		return std::fgetc(m_file);
	}
	bool ungetc(int character) noexcept {
		if (!m_file) {
			return false;
		}
		return (std::ungetc(character, m_file) == character);
	}

	long tell() noexcept {
		if (!m_file) {
			return -1L;
		}
		return ftell(m_file);
	}
	bool seek(long offset, int origin) noexcept {
		if (!m_file) {
			return false;
		}
		return (std::fseek(m_file, offset, origin) == 0);
	}
	bool skip(size_t size) noexcept {
		if (!m_file) {
			return false;
		}
		return (std::fseek(m_file, static_cast<long>(size), SEEK_CUR) == 0);
	}

	bool reverse(size_t size) noexcept {
		if (!m_file) {
			return false;
		}
		return (std::fseek(m_file, -static_cast<long>(size), SEEK_CUR) == 0);
	}

	bool getpos(fpos_t& pos) noexcept {
		if (!m_file) {
			return false;
		}
		return (std::fgetpos(m_file, &pos) == 0);
	}
	bool setpos(const fpos_t& pos) noexcept {
		if (!m_file) {
			return false;
		}
		return (std::fsetpos(m_file, &pos) == 0);
	}

private:
	void close_internal() noexcept {
		if (m_file) {
			std::fclose(m_file);
		}
	}

	FILE* m_file;
};
} // namespace fp

// std::swap specialization
namespace std {
template <>
inline void swap(fp::file& a, fp::file& b) noexcept {
	a.swap(b);
}
} // namespace std
