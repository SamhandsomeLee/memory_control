/**************************************************************************/
/*  error_handling.h                                                     */
/**************************************************************************/
/*  Independent Memory Management Module                                  */
/*  Error handling and assertion system                                  */
/**************************************************************************/

#pragma once

#include "platform_defines.h"
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <string>

// Error handler types
enum class MemoryErrorType {
    MEM_ERROR,
    MEM_WARNING,
    MEM_ASSERTION,
    MEM_FATAL
};

// Error handler function type
using MemoryErrorHandler = void(*)(MemoryErrorType type, const char* function, const char* file, int line, const char* message);

// Default error handler
MEMORY_NO_INLINE void default_memory_error_handler(MemoryErrorType type, const char* function, const char* file, int line, const char* message) {
    const char* type_str = "UNKNOWN";
    switch (type) {
        case MemoryErrorType::MEM_ERROR:
            type_str = "ERROR";
            break;
        case MemoryErrorType::MEM_WARNING:
            type_str = "WARNING";
            break;
        case MemoryErrorType::MEM_ASSERTION:
            type_str = "ASSERTION";
            break;
        case MemoryErrorType::MEM_FATAL:
            type_str = "FATAL";
            break;
    }
    
    std::cerr << "[" << type_str << "] " << function << " (" << file << ":" << line << "): " << message << std::endl;
    
    if (type == MemoryErrorType::MEM_FATAL || type == MemoryErrorType::MEM_ASSERTION) {
        std::abort();
    }
}

// Global error handler
namespace {
    MemoryErrorHandler g_memory_error_handler = default_memory_error_handler;
}

// Set custom error handler
MEMORY_ALWAYS_INLINE void set_memory_error_handler(MemoryErrorHandler handler) {
    g_memory_error_handler = handler ? handler : default_memory_error_handler;
}

// Get current error handler
MEMORY_ALWAYS_INLINE MemoryErrorHandler get_memory_error_handler() {
    return g_memory_error_handler;
}

// Internal error reporting function
MEMORY_NO_INLINE void _memory_report_error(MemoryErrorType type, const char* function, const char* file, int line, const char* message) {
    g_memory_error_handler(type, function, file, line, message);
}

// Assertion macro
#if MEMORY_DEBUG_ENABLED
#define MEMORY_ASSERT(condition, message) \
    do { \
        if (MEMORY_UNLIKELY(!(condition))) { \
            _memory_report_error(MemoryErrorType::MEM_ASSERTION, MEMORY_FUNCTION_STR, __FILE__, __LINE__, message); \
        } \
    } while (0)
#else
#define MEMORY_ASSERT(condition, message) ((void)0)
#endif

// Development assertion (always enabled in debug builds)
#define MEMORY_DEV_ASSERT(condition) \
    do { \
        if (MEMORY_DEBUG_ENABLED && MEMORY_UNLIKELY(!(condition))) { \
            _memory_report_error(MemoryErrorType::MEM_ASSERTION, MEMORY_FUNCTION_STR, __FILE__, __LINE__, "Assertion failed: " #condition); \
        } \
    } while (0)

// Error reporting macros
#define MEMORY_ERROR(message) \
    do { \
        _memory_report_error(MemoryErrorType::MEM_ERROR, MEMORY_FUNCTION_STR, __FILE__, __LINE__, message); \
    } while (0)

#define MEMORY_WARNING(message) \
    do { \
        _memory_report_error(MemoryErrorType::MEM_WARNING, MEMORY_FUNCTION_STR, __FILE__, __LINE__, message); \
    } while (0)

#define MEMORY_FATAL(message) \
    do { \
        _memory_report_error(MemoryErrorType::MEM_FATAL, MEMORY_FUNCTION_STR, __FILE__, __LINE__, message); \
    } while (0)

// Null pointer check macros
#define MEMORY_ERR_FAIL_NULL(ptr) \
    do { \
        if (MEMORY_UNLIKELY((ptr) == nullptr)) { \
            MEMORY_ERROR("Null pointer: " #ptr); \
            return; \
        } \
    } while (0)

#define MEMORY_ERR_FAIL_NULL_V(ptr, retval) \
    do { \
        if (MEMORY_UNLIKELY((ptr) == nullptr)) { \
            MEMORY_ERROR("Null pointer: " #ptr); \
            return (retval); \
        } \
    } while (0)

// Condition check macros
#define MEMORY_ERR_FAIL_COND(condition) \
    do { \
        if (MEMORY_UNLIKELY(condition)) { \
            MEMORY_ERROR("Condition failed: " #condition); \
            return; \
        } \
    } while (0)

#define MEMORY_ERR_FAIL_COND_V(condition, retval) \
    do { \
        if (MEMORY_UNLIKELY(condition)) { \
            MEMORY_ERROR("Condition failed: " #condition); \
            return (retval); \
        } \
    } while (0)

// Condition check with message
#define MEMORY_ERR_FAIL_COND_MSG(condition, message) \
    do { \
        if (MEMORY_UNLIKELY(condition)) { \
            MEMORY_ERROR(message); \
            return; \
        } \
    } while (0)

#define MEMORY_ERR_FAIL_COND_V_MSG(condition, retval, message) \
    do { \
        if (MEMORY_UNLIKELY(condition)) { \
            MEMORY_ERROR(message); \
            return (retval); \
        } \
    } while (0)

// Index bounds check macros
#define MEMORY_ERR_FAIL_INDEX(index, size) \
    do { \
        if (MEMORY_UNLIKELY((index) < 0 || (index) >= (size))) { \
            std::string msg = "Index out of bounds: " #index " = " + std::to_string(index) + ", size = " + std::to_string(size); \
            _memory_report_error(MemoryErrorType::MEM_ERROR, MEMORY_FUNCTION_STR, __FILE__, __LINE__, msg.c_str()); \
            return; \
        } \
    } while (0)

#define MEMORY_ERR_FAIL_INDEX_V(index, size, retval) \
    do { \
        if (MEMORY_UNLIKELY((index) < 0 || (index) >= (size))) { \
            std::string msg = "Index out of bounds: " #index " = " + std::to_string(index) + ", size = " + std::to_string(size); \
            _memory_report_error(MemoryErrorType::MEM_ERROR, MEMORY_FUNCTION_STR, __FILE__, __LINE__, msg.c_str()); \
            return (retval); \
        } \
    } while (0)

// Crash macro
#define MEMORY_CRASH_NOW() \
    do { \
        _memory_report_error(MemoryErrorType::MEM_FATAL, MEMORY_FUNCTION_STR, __FILE__, __LINE__, "Crash requested"); \
    } while (0)

#define MEMORY_CRASH_NOW_MSG(message) \
    do { \
        _memory_report_error(MemoryErrorType::MEM_FATAL, MEMORY_FUNCTION_STR, __FILE__, __LINE__, message); \
    } while (0)

// Debug-only assertions
#if MEMORY_DEBUG_ENABLED
#define MEMORY_DEBUG_ASSERT(condition) MEMORY_ASSERT(condition, "Debug assertion failed: " #condition)
#else
#define MEMORY_DEBUG_ASSERT(condition) ((void)0)
#endif 