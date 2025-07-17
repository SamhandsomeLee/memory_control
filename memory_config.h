/**************************************************************************/
/*  memory_config.h                                                      */
/**************************************************************************/
/*  Independent Memory Management Module                                  */
/*  Configuration system for memory management                           */
/**************************************************************************/

#pragma once

#include "platform_defines.h"
#include "thread_safe.h"

// Memory tracking levels
enum class MemoryTrackingLevel {
    NONE,       // No tracking
    BASIC,      // Basic tracking (total usage)
    DETAILED,   // Detailed tracking (per-allocation info)
    FULL        // Full tracking with call stacks
};

// Memory alignment policies
enum class MemoryAlignmentPolicy {
    NONE,           // No special alignment
    STANDARD,       // Standard alignment (max_align_t)
    CUSTOM,         // Custom alignment requirements
    PLATFORM_OPTIMAL // Platform-optimal alignment
};

// Memory padding policies
enum class MemoryPaddingPolicy {
    NONE,           // No padding
    DEBUG_ONLY,     // Padding only in debug builds
    ALWAYS,         // Always pad
    CONFIGURABLE    // Runtime configurable
};

// Memory allocation strategies
enum class MemoryAllocationStrategy {
    SYSTEM_DEFAULT,     // Use system malloc/free
    POOLED,            // Use memory pools
    CUSTOM,            // Custom allocator
    HYBRID             // Hybrid approach
};

// Error handling policies
enum class MemoryErrorPolicy {
    SILENT,         // No error reporting
    LOG_ONLY,       // Log errors but continue
    ASSERT_DEBUG,   // Assert in debug builds
    ASSERT_ALWAYS,  // Always assert
    EXCEPTION       // Throw exceptions
};

// Default configuration template
template<
    bool EnableTracking = MEMORY_DEBUG_ENABLED,
    bool EnableAlignment = true,
    bool EnablePadding = MEMORY_DEBUG_ENABLED,
    ThreadSafetyPolicy ThreadPolicy = ThreadSafetyPolicy::STD_ATOMIC,
    MemoryTrackingLevel TrackingLevel = EnableTracking ? MemoryTrackingLevel::BASIC : MemoryTrackingLevel::NONE,
    MemoryAlignmentPolicy AlignmentPolicy = MemoryAlignmentPolicy::STANDARD,
    MemoryPaddingPolicy PaddingPolicy = EnablePadding ? MemoryPaddingPolicy::DEBUG_ONLY : MemoryPaddingPolicy::NONE,
    MemoryAllocationStrategy AllocationStrategy = MemoryAllocationStrategy::SYSTEM_DEFAULT,
    MemoryErrorPolicy ErrorPolicy = MEMORY_DEBUG_ENABLED ? MemoryErrorPolicy::ASSERT_DEBUG : MemoryErrorPolicy::LOG_ONLY
>
struct MemoryConfig {
    // Core features
    static constexpr bool ENABLE_TRACKING = EnableTracking;
    static constexpr bool ENABLE_ALIGNMENT = EnableAlignment;
    static constexpr bool ENABLE_PADDING = EnablePadding;
    
    // Policies
    static constexpr ThreadSafetyPolicy THREAD_POLICY = ThreadPolicy;
    static constexpr MemoryTrackingLevel TRACKING_LEVEL = TrackingLevel;
    static constexpr MemoryAlignmentPolicy ALIGNMENT_POLICY = AlignmentPolicy;
    static constexpr MemoryPaddingPolicy PADDING_POLICY = PaddingPolicy;
    static constexpr MemoryAllocationStrategy ALLOCATION_STRATEGY = AllocationStrategy;
    static constexpr MemoryErrorPolicy ERROR_POLICY = ErrorPolicy;
    
    // Memory layout constants (similar to Godot's layout)
    static constexpr memory_size_t SIZE_OFFSET = 0;
    static constexpr memory_size_t ELEMENT_OFFSET = 
        ((SIZE_OFFSET + sizeof(memory_uint64_t)) % alignof(memory_uint64_t) == 0) ? 
        (SIZE_OFFSET + sizeof(memory_uint64_t)) : 
        ((SIZE_OFFSET + sizeof(memory_uint64_t)) + alignof(memory_uint64_t) - 
         ((SIZE_OFFSET + sizeof(memory_uint64_t)) % alignof(memory_uint64_t)));
    
    static constexpr memory_size_t DATA_OFFSET = 
        ((ELEMENT_OFFSET + sizeof(memory_uint64_t)) % alignof(std::max_align_t) == 0) ? 
        (ELEMENT_OFFSET + sizeof(memory_uint64_t)) : 
        ((ELEMENT_OFFSET + sizeof(memory_uint64_t)) + alignof(std::max_align_t) - 
         ((ELEMENT_OFFSET + sizeof(memory_uint64_t)) % alignof(std::max_align_t)));
    
    // Performance tuning
    static constexpr memory_size_t DEFAULT_ALIGNMENT = alignof(std::max_align_t);
    static constexpr memory_size_t CACHE_LINE_SIZE = 64; // Common cache line size
    static constexpr memory_size_t PAGE_SIZE = 4096;     // Common page size
    
    // Validation
    static_assert(ELEMENT_OFFSET >= SIZE_OFFSET + sizeof(memory_uint64_t), "Invalid element offset");
    static_assert(DATA_OFFSET >= ELEMENT_OFFSET + sizeof(memory_uint64_t), "Invalid data offset");
};

// Predefined configurations
using DefaultConfig = MemoryConfig<>;

using HighPerformanceConfig = MemoryConfig<
    false,                              // EnableTracking
    false,                              // EnableAlignment
    false,                              // EnablePadding
    ThreadSafetyPolicy::NONE,           // ThreadPolicy
    MemoryTrackingLevel::NONE,          // TrackingLevel
    MemoryAlignmentPolicy::NONE,        // AlignmentPolicy
    MemoryPaddingPolicy::NONE,          // PaddingPolicy
    MemoryAllocationStrategy::SYSTEM_DEFAULT, // AllocationStrategy
    MemoryErrorPolicy::SILENT          // ErrorPolicy
>;

using DebugConfig = MemoryConfig<
    true,                               // EnableTracking
    true,                               // EnableAlignment
    true,                               // EnablePadding
    ThreadSafetyPolicy::STD_ATOMIC,     // ThreadPolicy
    MemoryTrackingLevel::DETAILED,      // TrackingLevel
    MemoryAlignmentPolicy::STANDARD,    // AlignmentPolicy
    MemoryPaddingPolicy::ALWAYS,        // PaddingPolicy
    MemoryAllocationStrategy::SYSTEM_DEFAULT, // AllocationStrategy
    MemoryErrorPolicy::ASSERT_ALWAYS   // ErrorPolicy
>;

using EmbeddedConfig = MemoryConfig<
    false,                              // EnableTracking
    false,                              // EnableAlignment
    false,                              // EnablePadding
    ThreadSafetyPolicy::NONE,           // ThreadPolicy
    MemoryTrackingLevel::NONE,          // TrackingLevel
    MemoryAlignmentPolicy::NONE,        // AlignmentPolicy
    MemoryPaddingPolicy::NONE,          // PaddingPolicy
    MemoryAllocationStrategy::POOLED,   // AllocationStrategy
    MemoryErrorPolicy::SILENT          // ErrorPolicy
>;

using ThreadSafeConfig = MemoryConfig<
    true,                               // EnableTracking
    true,                               // EnableAlignment
    true,                               // EnablePadding
    ThreadSafetyPolicy::STD_ATOMIC,     // ThreadPolicy
    MemoryTrackingLevel::BASIC,         // TrackingLevel
    MemoryAlignmentPolicy::STANDARD,    // AlignmentPolicy
    MemoryPaddingPolicy::DEBUG_ONLY,    // PaddingPolicy
    MemoryAllocationStrategy::SYSTEM_DEFAULT, // AllocationStrategy
    MemoryErrorPolicy::ASSERT_DEBUG    // ErrorPolicy
>;

// Runtime configuration (for dynamic settings)
struct MemoryRuntimeConfig {
    // Hooks
    using AllocationHook = void(*)(void* ptr, memory_size_t size, const char* context);
    using DeallocationHook = void(*)(void* ptr, memory_size_t size, const char* context);
    using ReallocHook = void(*)(void* old_ptr, void* new_ptr, memory_size_t old_size, memory_size_t new_size, const char* context);
    
    // Runtime settings
    bool enable_hooks = false;
    AllocationHook allocation_hook = nullptr;
    DeallocationHook deallocation_hook = nullptr;
    ReallocHook realloc_hook = nullptr;
    
    // Memory limits
    memory_size_t max_memory_usage = 0; // 0 = unlimited
    memory_size_t warning_threshold = 0; // 0 = no warning
    
    // Debug settings
    bool enable_leak_detection = false;
    bool enable_double_free_detection = false;
    bool enable_bounds_checking = false;
    
    // Performance settings
    memory_size_t small_allocation_threshold = 256;
    memory_size_t large_allocation_threshold = 1024 * 1024; // 1MB
    
    // Singleton access
    static MemoryRuntimeConfig& instance() {
        static MemoryRuntimeConfig config;
        return config;
    }
};

// Configuration validation helpers
template<typename Config>
constexpr bool validate_config() {
    // Check for logical inconsistencies
    if constexpr (Config::ENABLE_TRACKING && Config::TRACKING_LEVEL == MemoryTrackingLevel::NONE) {
        return false;
    }
    
    if constexpr (Config::ENABLE_ALIGNMENT && Config::ALIGNMENT_POLICY == MemoryAlignmentPolicy::NONE) {
        return false;
    }
    
    if constexpr (Config::ENABLE_PADDING && Config::PADDING_POLICY == MemoryPaddingPolicy::NONE) {
        return false;
    }
    
    return true;
}

// Compile-time configuration validation
#define MEMORY_VALIDATE_CONFIG(Config) \
    static_assert(validate_config<Config>(), "Invalid memory configuration");

// Validate predefined configurations
MEMORY_VALIDATE_CONFIG(DefaultConfig);
MEMORY_VALIDATE_CONFIG(HighPerformanceConfig);
MEMORY_VALIDATE_CONFIG(DebugConfig);
MEMORY_VALIDATE_CONFIG(EmbeddedConfig);
MEMORY_VALIDATE_CONFIG(ThreadSafeConfig); 