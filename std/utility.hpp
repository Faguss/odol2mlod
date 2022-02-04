#pragma once
// exception handeling
#if __cpp_exceptions
#define FP_STD_TRY try
#define FP_STD_CATCH(x) catch (x)
#else
#define FP_STD_TRY if constexpr (true)
#define FP_STD_CATCH(x) if constexpr (false)
#endif

// debug handeling
#if (FP_STD_DEBUG_LEVEL >= 1)
#ifndef FP_STD_DEBUG_ASSERT
#define FP_STD_DEBUG_ASSERT(x, m)                                      \
	if (!(x)) {                                                        \
		fp::internal::debug::assert_failed(#x, m, __FILE__, __LINE__); \
	}
#include <cstdlib>
#include <cstdio>

namespace fp {
namespace internal {
namespace debug {
[[noreturn]] inline void assert_failed(const char* expr, const char* message, const char* file, int line) {
	std::fprintf(stderr, "helpers assert failed %s:%d: %s\ncondition: %s\n", file, line, message, expr);
	std::abort();
}
} // namespace debug
} // namespace internal
} // namespace fp
#endif
#else
#ifndef FP_STD_DEBUG_ASSERT
#define FP_STD_DEBUG_ASSERT(x, m) ((void) 0);
#endif
#endif

