/**************************************************************************/
/*  platform_defines.h                                                   */
/**************************************************************************/
/*  Independent Memory Management Module                                  */
/*  Platform-specific type definitions and macros                        */
/**************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <type_traits>

// Ensure C++14 or later for basic functionality
// Note: Some features may require C++17, consider upgrading your project settings
static_assert(__cplusplus >= 201402L, "Minimum of C++14 required. Consider upgrading to C++17 for full feature support.");

// Platform-specific inline macros
#ifndef MEMORY_ALWAYS_INLINE
#if defined(__GNUC__)
#define MEMORY_ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define MEMORY_ALWAYS_INLINE __forceinline
#else
#define MEMORY_ALWAYS_INLINE inline
#endif
#endif

#ifndef MEMORY_FORCE_INLINE
#if defined(DEBUG) || defined(_DEBUG)
#define MEMORY_FORCE_INLINE inline
#else
#define MEMORY_FORCE_INLINE MEMORY_ALWAYS_INLINE
#endif
#endif

#ifndef MEMORY_NO_INLINE
#if defined(__GNUC__)
#define MEMORY_NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define MEMORY_NO_INLINE __declspec(noinline)
#else
#define MEMORY_NO_INLINE
#endif
#endif

// Type aliases for consistency
using memory_size_t = std::size_t;
using memory_uint64_t = std::uint64_t;
using memory_uint32_t = std::uint32_t;
using memory_uint8_t = std::uint8_t;
using memory_uintptr_t = std::uintptr_t;

// Utility functions
template <typename T>
MEMORY_ALWAYS_INLINE bool is_power_of_2(const T x) {
    return x && ((x & (x - 1)) == 0);
}

// Function to find the next power of 2
constexpr memory_uint64_t next_power_of_2(memory_uint64_t p_number) {
    if (p_number == 0) {
        return 0;
    }
    
    --p_number;
    p_number |= p_number >> 1;
    p_number |= p_number >> 2;
    p_number |= p_number >> 4;
    p_number |= p_number >> 8;
    p_number |= p_number >> 16;
    p_number |= p_number >> 32;
    
    return ++p_number;
}

// Platform-specific alignment
#ifndef MEMORY_ALIGN
#if defined(__GNUC__) || defined(__clang__)
#define MEMORY_ALIGN(x) __attribute__((aligned(x)))
#elif defined(_MSC_VER)
#define MEMORY_ALIGN(x) __declspec(align(x))
#else
#define MEMORY_ALIGN(x)
#endif
#endif

// String manipulation macros
#ifndef MEMORY_STR
#define MEMORY_STR(m_x) #m_x
#define MEMORY_MKSTR(m_x) MEMORY_STR(m_x)
#endif

// Function name macro
#ifdef __GNUC__
#define MEMORY_FUNCTION_STR __FUNCTION__
#else
#define MEMORY_FUNCTION_STR __FUNCTION__
#endif

// Swap macro
#ifndef MEMORY_SWAP
#define MEMORY_SWAP(m_x, m_y) std::swap((m_x), (m_y))
#endif

// Likely/unlikely hints
#ifndef MEMORY_LIKELY
#if defined(__GNUC__) || defined(__clang__)
#define MEMORY_LIKELY(x) __builtin_expect(!!(x), 1)
#define MEMORY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define MEMORY_LIKELY(x) (x)
#define MEMORY_UNLIKELY(x) (x)
#endif
#endif

// Debug/Release detection
#ifndef MEMORY_DEBUG_ENABLED
#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
#define MEMORY_DEBUG_ENABLED 1
#else
#define MEMORY_DEBUG_ENABLED 0
#endif
#endif

// Platform detection
#ifndef MEMORY_PLATFORM_WINDOWS
#if defined(_WIN32) || defined(_WIN64)
#define MEMORY_PLATFORM_WINDOWS 1
#else
#define MEMORY_PLATFORM_WINDOWS 0
#endif
#endif

#ifndef MEMORY_PLATFORM_LINUX
#if defined(__linux__)
#define MEMORY_PLATFORM_LINUX 1
#else
#define MEMORY_PLATFORM_LINUX 0
#endif
#endif

#ifndef MEMORY_PLATFORM_MACOS
#if defined(__APPLE__) && defined(__MACH__)
#define MEMORY_PLATFORM_MACOS 1
#else
#define MEMORY_PLATFORM_MACOS 0
#endif
#endif

// Compiler detection
#ifndef MEMORY_COMPILER_MSVC
#if defined(_MSC_VER)
#define MEMORY_COMPILER_MSVC 1
#else
#define MEMORY_COMPILER_MSVC 0
#endif
#endif

#ifndef MEMORY_COMPILER_GCC
#if defined(__GNUC__) && !defined(__clang__)
#define MEMORY_COMPILER_GCC 1
#else
#define MEMORY_COMPILER_GCC 0
#endif
#endif

#ifndef MEMORY_COMPILER_CLANG
#if defined(__clang__)
#define MEMORY_COMPILER_CLANG 1
#else
#define MEMORY_COMPILER_CLANG 0
#endif
#endif 