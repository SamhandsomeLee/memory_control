/**************************************************************************/
/*  memory_manager.h                                                     */
/**************************************************************************/
/*  Independent Memory Management Module                                  */
/*  Core memory management implementation                                 */
/**************************************************************************/

#pragma once

#include "platform_defines.h"
#include "error_handling.h"
#include "thread_safe.h"
#include "memory_config.h"
#include "memory_tracker.h"
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>

// Forward declarations
template<typename Config = DefaultConfig>
class MemoryManager;

template<typename Config = DefaultConfig>
class DefaultAllocator;

// Core memory management implementation
template<typename Config>
class MemoryManager {
public:
    using TrackerType = MemoryTrackerType<Config>;

    // Memory layout constants from config
    static constexpr memory_size_t SIZE_OFFSET = Config::SIZE_OFFSET;
    static constexpr memory_size_t ELEMENT_OFFSET = Config::ELEMENT_OFFSET;
    static constexpr memory_size_t DATA_OFFSET = Config::DATA_OFFSET;

private:
    // Internal helper to check if we should use padding
    static constexpr bool should_use_padding(bool p_pad_align) {
        if constexpr (Config::PADDING_POLICY == MemoryPaddingPolicy::NONE) {
            return false;
        }
        else if constexpr (Config::PADDING_POLICY == MemoryPaddingPolicy::ALWAYS) {
            return true;
        }
        else if constexpr (Config::PADDING_POLICY == MemoryPaddingPolicy::DEBUG_ONLY) {
            return MEMORY_DEBUG_ENABLED;
        }
        else {
            return p_pad_align;
        }
    }

    // Internal helper to get size from padded memory
    static MEMORY_ALWAYS_INLINE memory_uint64_t* get_size_ptr(memory_uint8_t* p_mem) {
        return reinterpret_cast<memory_uint64_t*>(p_mem + SIZE_OFFSET);
    }

    // Internal helper to get element count from padded memory
    static MEMORY_ALWAYS_INLINE memory_uint64_t* get_element_count_ptr(memory_uint8_t* p_ptr) {
        return reinterpret_cast<memory_uint64_t*>(p_ptr - DATA_OFFSET + ELEMENT_OFFSET);
    }

public:
    // Core allocation function (template to enable zero-initialization)
    template<bool p_ensure_zero = false>
    static void* alloc_static(memory_size_t p_bytes, bool p_pad_align = false) {
        const bool prepad = should_use_padding(p_pad_align);

        void* mem;
        if constexpr (p_ensure_zero) {
            mem = std::calloc(1, p_bytes + (prepad ? DATA_OFFSET : 0));
        }
        else {
            mem = std::malloc(p_bytes + (prepad ? DATA_OFFSET : 0));
        }

        MEMORY_ERR_FAIL_NULL_V(mem, static_cast<void*>(nullptr));

        if (prepad) {
            memory_uint8_t* s8 = static_cast<memory_uint8_t*>(mem);
            memory_uint64_t* s = get_size_ptr(s8);
            *s = p_bytes;

            // Track allocation
            TrackerType::track_allocation(p_bytes, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

            return s8 + DATA_OFFSET;
        }
        else {
            // Track allocation
            TrackerType::track_allocation(p_bytes, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

            return mem;
        }
    }

    // Zero-initialized allocation
    static MEMORY_FORCE_INLINE void* alloc_static_zeroed(memory_size_t p_bytes, bool p_pad_align = false) {
        return alloc_static<true>(p_bytes, p_pad_align);
    }

    // Reallocation function
    static void* realloc_static(void* p_memory, memory_size_t p_bytes, bool p_pad_align = false) {
        if (p_memory == nullptr) {
            return alloc_static(p_bytes, p_pad_align);
        }

        memory_uint8_t* mem = static_cast<memory_uint8_t*>(p_memory);
        const bool prepad = should_use_padding(p_pad_align);

        if (prepad) {
            mem -= DATA_OFFSET;
            memory_uint64_t* s = get_size_ptr(mem);
            memory_size_t old_size = *s;

            // Track reallocation
            TrackerType::track_reallocation(old_size, p_bytes, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

            if (p_bytes == 0) {
                std::free(mem);
                return nullptr;
            }
            else {
                *s = p_bytes;

                mem = static_cast<memory_uint8_t*>(std::realloc(mem, p_bytes + DATA_OFFSET));
                MEMORY_ERR_FAIL_NULL_V(mem, static_cast<void*>(nullptr));

                s = get_size_ptr(mem);
                *s = p_bytes;

                return mem + DATA_OFFSET;
            }
        }
        else {
            // For non-padded allocations, we can't track the old size
            // This is a limitation of the simple approach
            TrackerType::track_reallocation(0, p_bytes, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

            mem = static_cast<memory_uint8_t*>(std::realloc(mem, p_bytes));
            MEMORY_ERR_FAIL_COND_V(mem == nullptr && p_bytes > 0, nullptr);

            return mem;
        }
    }

    // Free function
    static void free_static(void* p_ptr, bool p_pad_align = false) {
        MEMORY_ERR_FAIL_NULL(p_ptr);

        memory_uint8_t* mem = static_cast<memory_uint8_t*>(p_ptr);
        const bool prepad = should_use_padding(p_pad_align);

        if (prepad) {
            mem -= DATA_OFFSET;

            memory_uint64_t* s = get_size_ptr(mem);
            memory_size_t size = *s;

            // Track deallocation
            TrackerType::track_deallocation(size, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

            std::free(mem);
        }
        else {
            // For non-padded allocations, we can't track the size
            TrackerType::track_deallocation(0, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

            std::free(mem);
        }
    }

    // Aligned allocation functions (preserving Godot's algorithm)
    static void* alloc_aligned_static(memory_size_t p_bytes, memory_size_t p_alignment) {
        MEMORY_DEV_ASSERT(is_power_of_2(p_alignment));

        void* p1;
        void* p2;

        if ((p1 = std::malloc(p_bytes + p_alignment - 1 + sizeof(memory_uint32_t))) == nullptr) {
            return nullptr;
        }

        p2 = reinterpret_cast<void*>(
            (reinterpret_cast<memory_uintptr_t>(p1) + sizeof(memory_uint32_t) + p_alignment - 1) &
            ~(p_alignment - 1)
            );

        *(static_cast<memory_uint32_t*>(p2) - 1) =
            static_cast<memory_uint32_t>(
                reinterpret_cast<memory_uintptr_t>(p2) - reinterpret_cast<memory_uintptr_t>(p1)
                );

        // Track allocation
        TrackerType::track_allocation(p_bytes, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

        return p2;
    }

    static void* realloc_aligned_static(void* p_memory, memory_size_t p_bytes, memory_size_t p_prev_bytes, memory_size_t p_alignment) {
        if (p_memory == nullptr) {
            return alloc_aligned_static(p_bytes, p_alignment);
        }

        void* ret = alloc_aligned_static(p_bytes, p_alignment);
        if (ret) {
            std::memcpy(ret, p_memory, p_prev_bytes);
        }

        // Track reallocation
        TrackerType::track_reallocation(p_prev_bytes, p_bytes, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

        free_aligned_static(p_memory);
        return ret;
    }

    static void free_aligned_static(void* p_memory) {
        MEMORY_ERR_FAIL_NULL(p_memory);

        memory_uint32_t offset = *(static_cast<memory_uint32_t*>(p_memory) - 1);
        void* p = reinterpret_cast<void*>(
            static_cast<memory_uint8_t*>(p_memory) - offset
            );

        // Track deallocation (we can't know the size)
        TrackerType::track_deallocation(0, __FILE__, __LINE__, MEMORY_FUNCTION_STR);

        std::free(p);
    }

    // Memory statistics
    static memory_uint64_t get_mem_available() {
        return static_cast<memory_uint64_t>(-1); // 0xFFFF... (unlimited)
    }

    static memory_uint64_t get_mem_usage() {
        return TrackerType::get_current_usage();
    }

    static memory_uint64_t get_mem_max_usage() {
        return TrackerType::get_peak_usage();
    }

    static MemoryStats get_memory_stats() {
        return TrackerType::get_stats();
    }

    static void reset_memory_stats() {
        TrackerType::reset_stats();
    }

    static void dump_memory_allocations() {
        TrackerType::dump_allocations();
    }
};

// Default allocator implementation
template<typename Config>
class DefaultAllocator {
public:
    using ManagerType = MemoryManager<Config>;

    static MEMORY_FORCE_INLINE void* alloc(memory_size_t p_memory) {
        return ManagerType::alloc_static(p_memory, false);
    }

    static MEMORY_FORCE_INLINE void free(void* p_ptr) {
        ManagerType::free_static(p_ptr, false);
    }

    static MEMORY_FORCE_INLINE void* realloc(void* p_ptr, memory_size_t p_memory) {
        return ManagerType::realloc_static(p_ptr, p_memory, false);
    }
};

// Convenience type aliases
using Memory = MemoryManager<DefaultConfig>;
using FastMemory = MemoryManager<HighPerformanceConfig>;
using DebugMemory = MemoryManager<DebugConfig>;
using EmbeddedMemory = MemoryManager<EmbeddedConfig>;
using ThreadSafeMemory = MemoryManager<ThreadSafeConfig>;

// Operator new overloads (similar to Godot's implementation)
inline void* operator new(memory_size_t p_size, [[maybe_unused]] const char* p_description) {
    return Memory::alloc_static(p_size, false);
}

inline void* operator new(memory_size_t p_size, void* (*p_allocfunc)(memory_size_t p_size)) {
    return p_allocfunc(p_size);
}

inline void* operator new([[maybe_unused]] memory_size_t p_size, void* p_pointer, [[maybe_unused]] memory_size_t check, [[maybe_unused]] const char* p_description) {
    // Placement new - just return the pointer
    return p_pointer;
}

#if MEMORY_COMPILER_MSVC
// MSVC warnings suppression
inline void operator delete(void* p_mem, const char* p_description) {
    MEMORY_CRASH_NOW_MSG("Call to placement delete should not happen.");
}

inline void operator delete(void* p_mem, void* (*p_allocfunc)(memory_size_t p_size)) {
    MEMORY_CRASH_NOW_MSG("Call to placement delete should not happen.");
}

inline void operator delete(void* p_mem, void* p_pointer, memory_size_t check, const char* p_description) {
    MEMORY_CRASH_NOW_MSG("Call to placement delete should not happen.");
}
#endif

// Template specializations for different configurations
template<>
class MemoryManager<HighPerformanceConfig> {
public:
    // High-performance specialization with minimal overhead
    template<bool p_ensure_zero = false>
    static MEMORY_ALWAYS_INLINE void* alloc_static(memory_size_t p_bytes, [[maybe_unused]] bool p_pad_align = false) {
        void* mem;
        if constexpr (p_ensure_zero) {
            mem = std::calloc(1, p_bytes);
        }
        else {
            mem = std::malloc(p_bytes);
        }
        return mem; // No error checking for maximum performance
    }

    static MEMORY_ALWAYS_INLINE void* alloc_static_zeroed(memory_size_t p_bytes, bool p_pad_align = false) {
        return alloc_static<true>(p_bytes, p_pad_align);
    }

    static MEMORY_ALWAYS_INLINE void* realloc_static(void* p_memory, memory_size_t p_bytes, [[maybe_unused]] bool p_pad_align = false) {
        return std::realloc(p_memory, p_bytes);
    }

    static MEMORY_ALWAYS_INLINE void free_static(void* p_ptr, [[maybe_unused]] bool p_pad_align = false) {
        std::free(p_ptr);
    }

    static MEMORY_ALWAYS_INLINE void* alloc_aligned_static(memory_size_t p_bytes, memory_size_t p_alignment) {
        void* p1;
        void* p2;

        if ((p1 = std::malloc(p_bytes + p_alignment - 1 + sizeof(memory_uint32_t))) == nullptr) {
            return nullptr;
        }

        p2 = reinterpret_cast<void*>(
            (reinterpret_cast<memory_uintptr_t>(p1) + sizeof(memory_uint32_t) + p_alignment - 1) &
            ~(p_alignment - 1)
            );

        *(static_cast<memory_uint32_t*>(p2) - 1) =
            static_cast<memory_uint32_t>(
                reinterpret_cast<memory_uintptr_t>(p2) - reinterpret_cast<memory_uintptr_t>(p1)
                );

        return p2;
    }

    static MEMORY_ALWAYS_INLINE void* realloc_aligned_static(void* p_memory, memory_size_t p_bytes, memory_size_t p_prev_bytes, memory_size_t p_alignment) {
        if (p_memory == nullptr) {
            return alloc_aligned_static(p_bytes, p_alignment);
        }

        void* ret = alloc_aligned_static(p_bytes, p_alignment);
        if (ret) {
            std::memcpy(ret, p_memory, p_prev_bytes);
        }
        free_aligned_static(p_memory);
        return ret;
    }

    static MEMORY_ALWAYS_INLINE void free_aligned_static(void* p_memory) {
        memory_uint32_t offset = *(static_cast<memory_uint32_t*>(p_memory) - 1);
        void* p = reinterpret_cast<void*>(static_cast<memory_uint8_t*>(p_memory) - offset);
        std::free(p);
    }

    static MEMORY_ALWAYS_INLINE memory_uint64_t get_mem_available() {
        return static_cast<memory_uint64_t>(-1);
    }

    static MEMORY_ALWAYS_INLINE memory_uint64_t get_mem_usage() {
        return 0;
    }

    static MEMORY_ALWAYS_INLINE memory_uint64_t get_mem_max_usage() {
        return 0;
    }

    static MEMORY_ALWAYS_INLINE MemoryStats get_memory_stats() {
        return MemoryStats{};
    }

    static MEMORY_ALWAYS_INLINE void reset_memory_stats() {
        // No-op
    }

    static MEMORY_ALWAYS_INLINE void dump_memory_allocations() {
        // No-op
    }
};