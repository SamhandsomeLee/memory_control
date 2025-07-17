/**************************************************************************/
/*  memory_tracker.h                                                     */
/**************************************************************************/
/*  Independent Memory Management Module                                  */
/*  Memory tracking and statistics                                       */
/**************************************************************************/

#pragma once

#include "platform_defines.h"
#include "error_handling.h"
#include "thread_safe.h"
#include "memory_config.h"
#include <unordered_map>
#include <string>
#include <mutex>

// Forward declarations
template<typename Config>
class MemoryTracker;

// Allocation information for detailed tracking
struct AllocationInfo {
    memory_size_t size;
    const char* file;
    int line;
    const char* function;
    memory_uint64_t timestamp;
    memory_uint64_t allocation_id;
    
    AllocationInfo() = default;
    AllocationInfo(memory_size_t s, const char* f, int l, const char* func, memory_uint64_t ts, memory_uint64_t id)
        : size(s), file(f), line(l), function(func), timestamp(ts), allocation_id(id) {}
};

// Memory statistics
struct MemoryStats {
    memory_uint64_t total_allocated = 0;
    memory_uint64_t total_freed = 0;
    memory_uint64_t current_usage = 0;
    memory_uint64_t peak_usage = 0;
    memory_uint64_t allocation_count = 0;
    memory_uint64_t deallocation_count = 0;
    memory_uint64_t reallocation_count = 0;
    
    void reset() {
        total_allocated = 0;
        total_freed = 0;
        current_usage = 0;
        peak_usage = 0;
        allocation_count = 0;
        deallocation_count = 0;
        reallocation_count = 0;
    }
};

// No tracking implementation
template<typename Config>
class MemoryTracker {
    static_assert(Config::TRACKING_LEVEL == MemoryTrackingLevel::NONE, "Use specialized tracker for non-none tracking");
    
public:
    using CounterType = SafeNumeric<memory_uint64_t, ThreadSafetyPolicy::NONE>;
    
    static MEMORY_ALWAYS_INLINE void track_allocation([[maybe_unused]] memory_size_t size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        // No-op
    }
    
    static MEMORY_ALWAYS_INLINE void track_deallocation([[maybe_unused]] memory_size_t size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        // No-op
    }
    
    static MEMORY_ALWAYS_INLINE void track_reallocation([[maybe_unused]] memory_size_t old_size, [[maybe_unused]] memory_size_t new_size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        // No-op
    }
    
    static MEMORY_ALWAYS_INLINE memory_uint64_t get_current_usage() {
        return 0;
    }
    
    static MEMORY_ALWAYS_INLINE memory_uint64_t get_peak_usage() {
        return 0;
    }
    
    static MEMORY_ALWAYS_INLINE memory_uint64_t get_allocation_count() {
        return 0;
    }
    
    static MEMORY_ALWAYS_INLINE MemoryStats get_stats() {
        return MemoryStats{};
    }
    
    static MEMORY_ALWAYS_INLINE void reset_stats() {
        // No-op
    }
    
    static MEMORY_ALWAYS_INLINE void dump_allocations() {
        // No-op
    }
};

// Basic tracking implementation - template specialization
template<typename Config>
class BasicMemoryTracker {
    static_assert(Config::TRACKING_LEVEL == MemoryTrackingLevel::BASIC, "Use for basic tracking only");
    
public:
    using CounterType = SafeNumeric<memory_uint64_t, Config::THREAD_POLICY>;
    
private:
    static CounterType current_usage_;
    static CounterType peak_usage_;
    static CounterType allocation_count_;
    static CounterType deallocation_count_;
    static CounterType reallocation_count_;
    
public:
    static MEMORY_ALWAYS_INLINE void track_allocation(memory_size_t size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        if constexpr (Config::ENABLE_TRACKING) {
            memory_uint64_t new_usage = current_usage_.add(size);
            peak_usage_.exchange_if_greater(new_usage);
            allocation_count_.increment();
        }
    }
    
    static MEMORY_ALWAYS_INLINE void track_deallocation(memory_size_t size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        if constexpr (Config::ENABLE_TRACKING) {
            current_usage_.sub(size);
            deallocation_count_.increment();
        }
    }
    
    static MEMORY_ALWAYS_INLINE void track_reallocation(memory_size_t old_size, memory_size_t new_size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        if constexpr (Config::ENABLE_TRACKING) {
            if (new_size > old_size) {
                memory_uint64_t new_usage = current_usage_.add(new_size - old_size);
                peak_usage_.exchange_if_greater(new_usage);
            } else if (old_size > new_size) {
                current_usage_.sub(old_size - new_size);
            }
            reallocation_count_.increment();
        }
    }
    
    static MEMORY_ALWAYS_INLINE memory_uint64_t get_current_usage() {
        if constexpr (Config::ENABLE_TRACKING) {
            return current_usage_.get();
        } else {
            return 0;
        }
    }
    
    static MEMORY_ALWAYS_INLINE memory_uint64_t get_peak_usage() {
        if constexpr (Config::ENABLE_TRACKING) {
            return peak_usage_.get();
        } else {
            return 0;
        }
    }
    
    static MEMORY_ALWAYS_INLINE memory_uint64_t get_allocation_count() {
        if constexpr (Config::ENABLE_TRACKING) {
            return allocation_count_.get();
        } else {
            return 0;
        }
    }
    
    static MemoryStats get_stats() {
        if constexpr (Config::ENABLE_TRACKING) {
            MemoryStats stats;
            stats.current_usage = current_usage_.get();
            stats.peak_usage = peak_usage_.get();
            stats.allocation_count = allocation_count_.get();
            stats.deallocation_count = deallocation_count_.get();
            stats.reallocation_count = reallocation_count_.get();
            stats.total_allocated = stats.allocation_count; // Simplified
            stats.total_freed = stats.deallocation_count;   // Simplified
            return stats;
        } else {
            return MemoryStats{};
        }
    }
    
    static void reset_stats() {
        if constexpr (Config::ENABLE_TRACKING) {
            current_usage_.set(0);
            peak_usage_.set(0);
            allocation_count_.set(0);
            deallocation_count_.set(0);
            reallocation_count_.set(0);
        }
    }
    
    static void dump_allocations() {
        // Basic tracking doesn't store individual allocations
        if constexpr (Config::ENABLE_TRACKING) {
            MemoryStats stats = get_stats();
            // Use error handler to output stats
            std::string message = "Memory Stats - Current: " + std::to_string(stats.current_usage) + 
                                 " Peak: " + std::to_string(stats.peak_usage) + 
                                 " Allocs: " + std::to_string(stats.allocation_count);
            _memory_report_error(MemoryErrorType::MEM_WARNING, MEMORY_FUNCTION_STR, __FILE__, __LINE__, message.c_str());
        }
    }
};

// Detailed tracking implementation
template<typename Config>
class DetailedMemoryTracker {
    static_assert(Config::TRACKING_LEVEL == MemoryTrackingLevel::DETAILED, "Use for detailed tracking only");
    
public:
    using CounterType = SafeNumeric<memory_uint64_t, Config::THREAD_POLICY>;
    
private:
    static CounterType current_usage_;
    static CounterType peak_usage_;
    static CounterType allocation_count_;
    static CounterType deallocation_count_;
    static CounterType reallocation_count_;
    static CounterType next_allocation_id_;
    
    // Thread-safe allocation tracking
    static std::mutex allocations_mutex_;
    static std::unordered_map<void*, AllocationInfo> allocations_;
    
public:
    static void track_allocation(memory_size_t size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        if constexpr (Config::ENABLE_TRACKING) {
            memory_uint64_t new_usage = current_usage_.add(size);
            peak_usage_.exchange_if_greater(new_usage);
            allocation_count_.increment();
            
            // Store allocation info if detailed tracking is enabled
            if constexpr (Config::TRACKING_LEVEL == MemoryTrackingLevel::DETAILED) {
                            [[maybe_unused]] memory_uint64_t alloc_id = next_allocation_id_.increment();
            // Note: We would need the actual pointer to store this properly
            // This is a simplified version
            }
        }
    }
    
    static void track_allocation_with_ptr(void* ptr, memory_size_t size, const char* file = nullptr, int line = 0, const char* function = nullptr) {
        if constexpr (Config::ENABLE_TRACKING && Config::TRACKING_LEVEL == MemoryTrackingLevel::DETAILED) {
            memory_uint64_t new_usage = current_usage_.add(size);
            peak_usage_.exchange_if_greater(new_usage);
            allocation_count_.increment();
            
            memory_uint64_t alloc_id = next_allocation_id_.increment();
            memory_uint64_t timestamp = 0; // Would need actual timestamp implementation
            
            std::lock_guard<std::mutex> lock(allocations_mutex_);
            allocations_[ptr] = AllocationInfo(size, file, line, function, timestamp, alloc_id);
        }
    }
    
    static void track_deallocation(memory_size_t size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        if constexpr (Config::ENABLE_TRACKING) {
            current_usage_.sub(size);
            deallocation_count_.increment();
        }
    }
    
    static void track_deallocation_with_ptr(void* ptr, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        if constexpr (Config::ENABLE_TRACKING && Config::TRACKING_LEVEL == MemoryTrackingLevel::DETAILED) {
            std::lock_guard<std::mutex> lock(allocations_mutex_);
            auto it = allocations_.find(ptr);
            if (it != allocations_.end()) {
                current_usage_.sub(it->second.size);
                allocations_.erase(it);
            }
            deallocation_count_.increment();
        }
    }
    
    static void track_reallocation(memory_size_t old_size, memory_size_t new_size, [[maybe_unused]] const char* file = nullptr, [[maybe_unused]] int line = 0, [[maybe_unused]] const char* function = nullptr) {
        if constexpr (Config::ENABLE_TRACKING) {
            if (new_size > old_size) {
                memory_uint64_t new_usage = current_usage_.add(new_size - old_size);
                peak_usage_.exchange_if_greater(new_usage);
            } else if (old_size > new_size) {
                current_usage_.sub(old_size - new_size);
            }
            reallocation_count_.increment();
        }
    }
    
    static memory_uint64_t get_current_usage() {
        if constexpr (Config::ENABLE_TRACKING) {
            return current_usage_.get();
        } else {
            return 0;
        }
    }
    
    static memory_uint64_t get_peak_usage() {
        if constexpr (Config::ENABLE_TRACKING) {
            return peak_usage_.get();
        } else {
            return 0;
        }
    }
    
    static memory_uint64_t get_allocation_count() {
        if constexpr (Config::ENABLE_TRACKING) {
            return allocation_count_.get();
        } else {
            return 0;
        }
    }
    
    static MemoryStats get_stats() {
        if constexpr (Config::ENABLE_TRACKING) {
            MemoryStats stats;
            stats.current_usage = current_usage_.get();
            stats.peak_usage = peak_usage_.get();
            stats.allocation_count = allocation_count_.get();
            stats.deallocation_count = deallocation_count_.get();
            stats.reallocation_count = reallocation_count_.get();
            stats.total_allocated = stats.allocation_count; // Simplified
            stats.total_freed = stats.deallocation_count;   // Simplified
            return stats;
        } else {
            return MemoryStats{};
        }
    }
    
    static void reset_stats() {
        if constexpr (Config::ENABLE_TRACKING) {
            current_usage_.set(0);
            peak_usage_.set(0);
            allocation_count_.set(0);
            deallocation_count_.set(0);
            reallocation_count_.set(0);
            next_allocation_id_.set(0);
            
            if constexpr (Config::TRACKING_LEVEL == MemoryTrackingLevel::DETAILED) {
                std::lock_guard<std::mutex> lock(allocations_mutex_);
                allocations_.clear();
            }
        }
    }
    
    static void dump_allocations() {
        if constexpr (Config::ENABLE_TRACKING && Config::TRACKING_LEVEL == MemoryTrackingLevel::DETAILED) {
            std::lock_guard<std::mutex> lock(allocations_mutex_);
            for (const auto& [ptr, info] : allocations_) {
                std::string message = "Leak: " + std::to_string(info.size) + " bytes at " + 
                                     (info.file ? info.file : "unknown") + ":" + std::to_string(info.line);
                _memory_report_error(MemoryErrorType::WARNING, info.function ? info.function : "unknown", 
                                   info.file ? info.file : "unknown", info.line, message.c_str());
            }
        }
    }
};

// Static member definitions would go in a .cpp file
// For header-only implementation, we use inline static
template<typename Config>
inline typename BasicMemoryTracker<Config>::CounterType BasicMemoryTracker<Config>::current_usage_{};

template<typename Config>
inline typename BasicMemoryTracker<Config>::CounterType BasicMemoryTracker<Config>::peak_usage_{};

template<typename Config>
inline typename BasicMemoryTracker<Config>::CounterType BasicMemoryTracker<Config>::allocation_count_{};

template<typename Config>
inline typename BasicMemoryTracker<Config>::CounterType BasicMemoryTracker<Config>::deallocation_count_{};

template<typename Config>
inline typename BasicMemoryTracker<Config>::CounterType BasicMemoryTracker<Config>::reallocation_count_{};

// Static member definitions for DetailedMemoryTracker
template<typename Config>
inline typename DetailedMemoryTracker<Config>::CounterType DetailedMemoryTracker<Config>::current_usage_{};

template<typename Config>
inline typename DetailedMemoryTracker<Config>::CounterType DetailedMemoryTracker<Config>::peak_usage_{};

template<typename Config>
inline typename DetailedMemoryTracker<Config>::CounterType DetailedMemoryTracker<Config>::allocation_count_{};

template<typename Config>
inline typename DetailedMemoryTracker<Config>::CounterType DetailedMemoryTracker<Config>::deallocation_count_{};

template<typename Config>
inline typename DetailedMemoryTracker<Config>::CounterType DetailedMemoryTracker<Config>::reallocation_count_{};

template<typename Config>
inline typename DetailedMemoryTracker<Config>::CounterType DetailedMemoryTracker<Config>::next_allocation_id_{};

template<typename Config>
inline std::mutex DetailedMemoryTracker<Config>::allocations_mutex_{};

template<typename Config>
inline std::unordered_map<void*, AllocationInfo> DetailedMemoryTracker<Config>::allocations_{};

// Type selection based on configuration
template<typename Config>
using MemoryTrackerType = std::conditional_t<
    Config::TRACKING_LEVEL == MemoryTrackingLevel::NONE,
    MemoryTracker<Config>,
    std::conditional_t<
        Config::TRACKING_LEVEL == MemoryTrackingLevel::BASIC,
        BasicMemoryTracker<Config>,
        DetailedMemoryTracker<Config>
    >
>; 