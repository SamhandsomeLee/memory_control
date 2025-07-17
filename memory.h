/**************************************************************************/
/*  memory.h                                                             */
/**************************************************************************/
/*  Independent Memory Management Module                                  */
/*  Main header file - include this to use the memory management system  */
/**************************************************************************/

#pragma once

// Core platform and configuration headers
#include "platform_defines.h"
#include "error_handling.h"
#include "thread_safe.h"
#include "memory_config.h"

// Memory management implementation
#include "memory_tracker.h"
#include "memory_manager.h"
#include "memory_interface.h"

// Version information
#define MEMORY_MODULE_VERSION_MAJOR 1
#define MEMORY_MODULE_VERSION_MINOR 0
#define MEMORY_MODULE_VERSION_PATCH 0

#define MEMORY_MODULE_VERSION_STRING "1.0.0"

// Convenience macros for version checking
#define MEMORY_MODULE_VERSION_CHECK(major, minor, patch) \
    ((MEMORY_MODULE_VERSION_MAJOR > (major)) || \
     (MEMORY_MODULE_VERSION_MAJOR == (major) && MEMORY_MODULE_VERSION_MINOR > (minor)) || \
     (MEMORY_MODULE_VERSION_MAJOR == (major) && MEMORY_MODULE_VERSION_MINOR == (minor) && MEMORY_MODULE_VERSION_PATCH >= (patch)))

// Module information
namespace memory_module {
    constexpr const char* get_version() {
        return MEMORY_MODULE_VERSION_STRING;
    }
    
    constexpr int get_version_major() {
        return MEMORY_MODULE_VERSION_MAJOR;
    }
    
    constexpr int get_version_minor() {
        return MEMORY_MODULE_VERSION_MINOR;
    }
    
    constexpr int get_version_patch() {
        return MEMORY_MODULE_VERSION_PATCH;
    }
    
    // Module initialization (optional)
    inline void initialize() {
        // Set up default error handler if needed
        // Initialize any global state
        // This is called automatically when the module is first used
    }
    
    // Module cleanup (optional)
    inline void finalize() {
        // Clean up any global state
        // Dump memory leaks if in debug mode
        if constexpr (MEMORY_DEBUG_ENABLED) {
            memory::dump_allocations();
        }
    }
    
    // Module information
    inline void print_info() {
        // Use the error handler to print information
        std::string info = "Memory Module v" + std::string(get_version()) + 
                          " - Platform: " + 
                          (MEMORY_PLATFORM_WINDOWS ? "Windows" : 
                           MEMORY_PLATFORM_LINUX ? "Linux" : 
                           MEMORY_PLATFORM_MACOS ? "macOS" : "Unknown") +
                          " - Compiler: " +
                          (MEMORY_COMPILER_MSVC ? "MSVC" : 
                           MEMORY_COMPILER_GCC ? "GCC" : 
                           MEMORY_COMPILER_CLANG ? "Clang" : "Unknown") +
                          " - Debug: " + (MEMORY_DEBUG_ENABLED ? "ON" : "OFF");
        
        _memory_report_error(MemoryErrorType::MEM_WARNING, MEMORY_FUNCTION_STR, __FILE__, __LINE__, info.c_str());
    }
}

// Automatic initialization
namespace {
    struct MemoryModuleInitializer {
        MemoryModuleInitializer() {
            memory_module::initialize();
        }
        
        ~MemoryModuleInitializer() {
            memory_module::finalize();
        }
    };
    
    static MemoryModuleInitializer g_memory_module_initializer;
}

// Configuration validation at compile time
static_assert(MEMORY_MODULE_VERSION_MAJOR >= 1, "Invalid version");
static_assert(sizeof(void*) >= 4, "Unsupported architecture");
static_assert(sizeof(memory_size_t) >= 4, "Invalid size_t size");

// Feature detection
#if MEMORY_DEBUG_ENABLED
    #define MEMORY_MODULE_FEATURE_DEBUG 1
#else
    #define MEMORY_MODULE_FEATURE_DEBUG 0
#endif

#if defined(MEMORY_SAFE_NUMERIC_TYPE_PUN_GUARANTEES)
    #define MEMORY_MODULE_FEATURE_THREAD_SAFE 1
#else
    #define MEMORY_MODULE_FEATURE_THREAD_SAFE 0
#endif

// Usage examples in comments:
/*
// Basic usage (Godot-compatible):
int* ptr = (int*)memalloc(sizeof(int) * 10);
memfree(ptr);

// Object allocation:
MyClass* obj = memnew(MyClass);
memdelete(obj);

// Array allocation:
MyClass* arr = memnew_arr(MyClass, 100);
memdelete_arr(arr);

// Modern C++ style:
auto ptr = memory::make_unique<MyClass>();
auto arr = memory::make_unique_array<int>(100);

// Different configurations:
void* fast_ptr = FastMemory::alloc_static(1024);
FastMemory::free_static(fast_ptr);

void* debug_ptr = DebugMemory::alloc_static(1024);
DebugMemory::free_static(debug_ptr);

// Memory statistics:
memory_uint64_t usage = memory::get_usage();
memory_uint64_t peak = memory::get_peak_usage();
MemoryStats stats = memory::get_stats();

// Runtime configuration:
memory::set_error_handler(my_error_handler);
auto& config = memory::get_runtime_config();
config.enable_hooks = true;
config.allocation_hook = my_allocation_hook;
*/ 