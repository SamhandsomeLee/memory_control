/**************************************************************************/
/*  thread_safe.h                                                        */
/**************************************************************************/
/*  Independent Memory Management Module                                  */
/*  Thread safety abstractions                                           */
/**************************************************************************/

#pragma once

#include "platform_defines.h"
#include "error_handling.h"
#include <atomic>
#include <type_traits>

// Thread safety policies
enum class ThreadSafetyPolicy {
    NONE,           // No thread safety, best performance
    STD_ATOMIC,     // Use std::atomic
    CUSTOM_ATOMIC   // Use custom atomic operations
};

// Forward declarations
template<typename T, ThreadSafetyPolicy Policy = ThreadSafetyPolicy::STD_ATOMIC>
class SafeNumeric;

template<ThreadSafetyPolicy Policy = ThreadSafetyPolicy::STD_ATOMIC>
class SafeFlag;

// No thread safety implementation
template<typename T>
class SafeNumeric<T, ThreadSafetyPolicy::NONE> {
private:
    T value;

public:
    SafeNumeric() : value(T{}) {}
    SafeNumeric(T initial_value) : value(initial_value) {}

    MEMORY_ALWAYS_INLINE void set(T p_value) {
        value = p_value;
    }

    MEMORY_ALWAYS_INLINE T get() const {
        return value;
    }

    MEMORY_ALWAYS_INLINE T increment() {
        return ++value;
    }

    MEMORY_ALWAYS_INLINE T postincrement() {
        return value++;
    }

    MEMORY_ALWAYS_INLINE T decrement() {
        return --value;
    }

    MEMORY_ALWAYS_INLINE T postdecrement() {
        return value--;
    }

    MEMORY_ALWAYS_INLINE T add(T p_value) {
        value += p_value;
        return value;
    }

    MEMORY_ALWAYS_INLINE T postadd(T p_value) {
        T old_value = value;
        value += p_value;
        return old_value;
    }

    MEMORY_ALWAYS_INLINE T sub(T p_value) {
        value -= p_value;
        return value;
    }

    MEMORY_ALWAYS_INLINE T postsub(T p_value) {
        T old_value = value;
        value -= p_value;
        return old_value;
    }

    MEMORY_ALWAYS_INLINE T exchange_if_greater(T p_value) {
        if (p_value > value) {
            value = p_value;
        }
        return value;
    }

    MEMORY_ALWAYS_INLINE T conditional_increment() {
        if (value == 0) {
            return 0;
        }
        return ++value;
    }
};

// std::atomic implementation
template<typename T>
class SafeNumeric<T, ThreadSafetyPolicy::STD_ATOMIC> {
private:
    std::atomic<T> value;

    // C++14 compatible check for lock-free atomic operations
    static_assert(std::is_trivially_copyable<T>::value, "Type must be trivially copyable for atomic operations");

public:
    SafeNumeric() : value(T{}) {}
    SafeNumeric(T initial_value) : value(initial_value) {}

    MEMORY_ALWAYS_INLINE void set(T p_value) {
        value.store(p_value, std::memory_order_release);
    }

    MEMORY_ALWAYS_INLINE T get() const {
        return value.load(std::memory_order_acquire);
    }

    MEMORY_ALWAYS_INLINE T increment() {
        return value.fetch_add(1, std::memory_order_acq_rel) + 1;
    }

    MEMORY_ALWAYS_INLINE T postincrement() {
        return value.fetch_add(1, std::memory_order_acq_rel);
    }

    MEMORY_ALWAYS_INLINE T decrement() {
        return value.fetch_sub(1, std::memory_order_acq_rel) - 1;
    }

    MEMORY_ALWAYS_INLINE T postdecrement() {
        return value.fetch_sub(1, std::memory_order_acq_rel);
    }

    MEMORY_ALWAYS_INLINE T add(T p_value) {
        return value.fetch_add(p_value, std::memory_order_acq_rel) + p_value;
    }

    MEMORY_ALWAYS_INLINE T postadd(T p_value) {
        return value.fetch_add(p_value, std::memory_order_acq_rel);
    }

    MEMORY_ALWAYS_INLINE T sub(T p_value) {
        return value.fetch_sub(p_value, std::memory_order_acq_rel) - p_value;
    }

    MEMORY_ALWAYS_INLINE T postsub(T p_value) {
        return value.fetch_sub(p_value, std::memory_order_acq_rel);
    }

    MEMORY_ALWAYS_INLINE T exchange_if_greater(T p_value) {
        T current = value.load(std::memory_order_acquire);
        while (p_value > current) {
            if (value.compare_exchange_weak(current, p_value, std::memory_order_acq_rel, std::memory_order_acquire)) {
                return p_value;
            }
        }
        return current;
    }

    MEMORY_ALWAYS_INLINE T conditional_increment() {
        T current = value.load(std::memory_order_acquire);
        while (current != 0) {
            if (value.compare_exchange_weak(current, current + 1, std::memory_order_acq_rel, std::memory_order_acquire)) {
                return current + 1;
            }
        }
        return 0;
    }
};

// Custom atomic implementation (placeholder for platform-specific optimizations)
template<typename T>
class SafeNumeric<T, ThreadSafetyPolicy::CUSTOM_ATOMIC> : public SafeNumeric<T, ThreadSafetyPolicy::STD_ATOMIC> {
    // For now, delegate to std::atomic
    // Can be specialized for specific platforms/types if needed
};

// SafeFlag implementations
template<>
class SafeFlag<ThreadSafetyPolicy::NONE> {
private:
    bool flag;

public:
    SafeFlag() : flag(false) {}

    MEMORY_ALWAYS_INLINE void set() {
        flag = true;
    }

    MEMORY_ALWAYS_INLINE bool is_set() const {
        return flag;
    }

    MEMORY_ALWAYS_INLINE void clear() {
        flag = false;
    }

    MEMORY_ALWAYS_INLINE bool test_and_set() {
        bool old_value = flag;
        flag = true;
        return old_value;
    }
};

template<>
class SafeFlag<ThreadSafetyPolicy::STD_ATOMIC> {
private:
    std::atomic<bool> flag;

public:
    SafeFlag() : flag(false) {}

    MEMORY_ALWAYS_INLINE void set() {
        flag.store(true, std::memory_order_release);
    }

    MEMORY_ALWAYS_INLINE bool is_set() const {
        return flag.load(std::memory_order_acquire);
    }

    MEMORY_ALWAYS_INLINE void clear() {
        flag.store(false, std::memory_order_release);
    }

    MEMORY_ALWAYS_INLINE bool test_and_set() {
        return flag.exchange(true, std::memory_order_acq_rel);
    }
};

template<>
class SafeFlag<ThreadSafetyPolicy::CUSTOM_ATOMIC> : public SafeFlag<ThreadSafetyPolicy::STD_ATOMIC> {
    // For now, delegate to std::atomic
};

// Type traits for safe numeric types
template<typename T>
struct is_safe_numeric : std::false_type {};

template<typename T, ThreadSafetyPolicy Policy>
struct is_safe_numeric<SafeNumeric<T, Policy>> : std::true_type {};

template<typename T>
inline constexpr bool is_safe_numeric_v = is_safe_numeric<T>::value;

// Convenience aliases
using SafeNumeric32 = SafeNumeric<memory_uint32_t>;
using SafeNumeric64 = SafeNumeric<memory_uint64_t>;
using SafeNumericSize = SafeNumeric<memory_size_t>;

// Thread-safe counter for different policies
template<ThreadSafetyPolicy Policy>
using ThreadSafeCounter = SafeNumeric<memory_uint64_t, Policy>;

// Type punning guarantees (similar to Godot's SAFE_NUMERIC_TYPE_PUN_GUARANTEES)
#define MEMORY_SAFE_NUMERIC_TYPE_PUN_GUARANTEES(m_type) \
    static_assert(sizeof(SafeNumeric<m_type, ThreadSafetyPolicy::STD_ATOMIC>) == sizeof(std::atomic<m_type>)); \
    static_assert(alignof(SafeNumeric<m_type, ThreadSafetyPolicy::STD_ATOMIC>) == alignof(std::atomic<m_type>)); \
    static_assert(std::is_trivially_destructible_v<std::atomic<m_type>>);

#define MEMORY_SAFE_FLAG_TYPE_PUN_GUARANTEES(policy) \
    static_assert(sizeof(SafeFlag<policy>) == sizeof(std::atomic<bool>)); \
    static_assert(alignof(SafeFlag<policy>) == alignof(std::atomic<bool>));

// Verify type punning for common types
MEMORY_SAFE_NUMERIC_TYPE_PUN_GUARANTEES(memory_uint32_t);
MEMORY_SAFE_NUMERIC_TYPE_PUN_GUARANTEES(memory_uint64_t);
MEMORY_SAFE_NUMERIC_TYPE_PUN_GUARANTEES(memory_size_t); 