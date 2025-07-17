# Memory Control Module

A high-performance, configurable memory management system inspired by Godot Engine's memory management architecture. This module provides both traditional C-style and modern C++ memory management interfaces with comprehensive tracking, debugging, and optimization capabilities.

## 🚀 Features

### Core Features
- **Multi-platform support**: Windows, Linux, macOS
- **Multi-compiler support**: MSVC, GCC, Clang
- **Configurable memory policies**: Different configurations for debug, release, embedded, and high-performance scenarios
- **Thread-safe operations**: Optional thread safety with configurable policies
- **Memory tracking**: Comprehensive allocation tracking and statistics
- **Error handling**: Robust error reporting and assertion system

### Memory Management Interfaces
- **Godot-compatible macros**: `memalloc`, `memfree`, `memnew`, `memdelete`
- **Modern C++ RAII**: `memory::unique_ptr`, `memory::unique_array`
- **Template-based configurations**: Different memory managers for different use cases
- **Aligned allocation**: Support for custom memory alignment requirements

### Debugging & Profiling
- **Memory leak detection**: Automatic leak detection in debug builds
- **Allocation tracking**: Detailed tracking with file, line, and function information
- **Statistics reporting**: Current usage, peak usage, allocation counts
- **Runtime configuration**: Dynamic configuration of memory policies

## 📁 Project Structure

```
memory_control/
├── memory.h              # Main header - include this to use the system
├── memory_interface.h    # User-friendly interface layer
├── memory_manager.h      # Core memory management implementation
├── memory_tracker.h      # Memory tracking and statistics
├── memory_config.h       # Configuration system
├── platform_defines.h    # Platform-specific definitions
├── error_handling.h      # Error handling and assertion system
├── thread_safe.h         # Thread safety abstractions
└── README.md            # This file
```

## 🛠️ Quick Start

### Basic Usage

```cpp
#include "memory_control/memory.h"

int main() {
    // Basic allocation (Godot-style)
    int* ptr = (int*)memalloc(sizeof(int) * 10);
    memfree(ptr);
    
    // Object allocation
    MyClass* obj = memnew(MyClass);
    memdelete(obj);
    
    // Array allocation
    MyClass* arr = memnew_arr(MyClass, 100);
    memdelete_arr(arr);
    
    // Modern C++ style
    auto unique_ptr = memory::make_unique<MyClass>();
    auto unique_arr = memory::make_unique_array<int>(100);
    
    return 0;
}
```

### Different Configurations

```cpp
// High-performance configuration (no tracking, no safety checks)
void* fast_ptr = FastMemory::alloc_static(1024);
FastMemory::free_static(fast_ptr);

// Debug configuration (full tracking and safety checks)
void* debug_ptr = DebugMemory::alloc_static(1024);
DebugMemory::free_static(debug_ptr);

// Thread-safe configuration
void* safe_ptr = ThreadSafeMemory::alloc_static(1024);
ThreadSafeMemory::free_static(safe_ptr);
```

### Memory Statistics

```cpp
// Get current memory usage
memory_uint64_t usage = memory::get_usage();
memory_uint64_t peak = memory::get_peak_usage();

// Get detailed statistics
MemoryStats stats = memory::get_stats();
std::cout << "Total allocated: " << stats.total_allocated << " bytes\n";
std::cout << "Current usage: " << stats.current_usage << " bytes\n";
std::cout << "Peak usage: " << stats.peak_usage << " bytes\n";
std::cout << "Allocation count: " << stats.allocation_count << "\n";
```

## ⚙️ Configuration

### Predefined Configurations

The module provides several predefined configurations:

- **`DefaultConfig`**: Balanced configuration for general use
- **`HighPerformanceConfig`**: Maximum performance, minimal overhead
- **`DebugConfig`**: Full debugging and tracking capabilities
- **`EmbeddedConfig`**: Optimized for embedded systems
- **`ThreadSafeConfig`**: Thread-safe operations with basic tracking

### Custom Configuration

```cpp
// Define custom configuration
using MyConfig = MemoryConfig<
    true,                               // EnableTracking
    true,                               // EnableAlignment
    true,                               // EnablePadding
    ThreadSafetyPolicy::STD_ATOMIC,     // ThreadPolicy
    MemoryTrackingLevel::DETAILED,      // TrackingLevel
    MemoryAlignmentPolicy::STANDARD,    // AlignmentPolicy
    MemoryPaddingPolicy::DEBUG_ONLY,    // PaddingPolicy
    MemoryAllocationStrategy::SYSTEM_DEFAULT, // AllocationStrategy
    MemoryErrorPolicy::ASSERT_DEBUG     // ErrorPolicy
>;

// Use custom configuration
using MyMemory = MemoryManager<MyConfig>;
```

### Runtime Configuration

```cpp
// Set custom error handler
memory::set_error_handler(my_error_handler);

// Configure runtime settings
auto& config = memory::get_runtime_config();
config.enable_hooks = true;
config.allocation_hook = my_allocation_hook;
config.max_memory_usage = 1024 * 1024 * 100; // 100MB limit
config.enable_leak_detection = true;
```

## 🔧 Building

### Requirements
- **C++14** or later (C++17 recommended for full features)
- **Visual Studio 2022** (Windows)
- **GCC 6+** or **Clang 5+** (Linux/macOS)

### Windows (Visual Studio 2022)
1. Open the solution file
2. Set C++ Language Standard to **ISO C++17 Standard (/std:c++17)**
3. Build the project

### Linux/macOS
```bash
# Using GCC
g++ -std=c++17 -I. your_file.cpp -o your_program

# Using Clang
clang++ -std=c++17 -I. your_file.cpp -o your_program
```

## 📊 Performance Characteristics

### Memory Overhead
- **Default configuration**: ~16 bytes per allocation (for tracking)
- **High-performance configuration**: 0 bytes overhead
- **Debug configuration**: ~32 bytes per allocation (full tracking)

### Thread Safety
- **No thread safety**: Maximum performance, single-threaded only
- **std::atomic**: Standard thread safety with minimal overhead
- **Custom atomic**: Platform-specific optimizations (future feature)

### Allocation Performance
- **System malloc/free**: Direct system calls
- **Pooled allocation**: Fast allocation for small objects (future feature)
- **Custom allocator**: User-defined allocation strategies

## 🐛 Debugging Features

### Memory Leak Detection
```cpp
// Enable leak detection
auto& config = memory::get_runtime_config();
config.enable_leak_detection = true;

// Dump allocations at program end
memory::dump_allocations();
```

### Error Handling
```cpp
// Custom error handler
void my_error_handler(MemoryErrorType type, const char* function, 
                     const char* file, int line, const char* message) {
    std::cerr << "[" << type << "] " << function << " (" << file << ":" << line << "): " 
              << message << std::endl;
}

memory::set_error_handler(my_error_handler);
```

### Assertions
```cpp
// Debug assertions
MEMORY_ASSERT(ptr != nullptr, "Pointer should not be null");
MEMORY_DEV_ASSERT(size > 0);

// Error checking
MEMORY_ERR_FAIL_NULL(ptr);
MEMORY_ERR_FAIL_COND(size == 0);
```

## 🔄 Migration from Standard C++

### From `new`/`delete`
```cpp
// Old way
MyClass* obj = new MyClass();
delete obj;

// New way
MyClass* obj = memnew(MyClass);
memdelete(obj);

// Or modern C++
auto obj = memory::make_unique<MyClass>();
```

### From `malloc`/`free`
```cpp
// Old way
int* ptr = (int*)malloc(sizeof(int) * 10);
free(ptr);

// New way
int* ptr = (int*)memalloc(sizeof(int) * 10);
memfree(ptr);
```

### From `std::unique_ptr`
```cpp
// Old way
auto ptr = std::make_unique<MyClass>();

// New way
auto ptr = memory::make_unique<MyClass>();
```

## 📈 Version History

### Version 1.0.0
- Initial release
- Core memory management functionality
- Godot-compatible interface
- Multi-platform support
- Configurable memory policies
- Thread safety options
- Memory tracking and statistics
- Error handling system

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📄 License

This project is provided as-is for educational and development purposes. Feel free to use and modify according to your needs.

## 🙏 Acknowledgments

- Inspired by Godot Engine's memory management system
- Built with modern C++ best practices
- Designed for high-performance applications

## 📞 Support

For questions, issues, or contributions, please refer to the project documentation or create an issue in the repository.

---

**Note**: This memory management module is designed to be a drop-in replacement for standard memory allocation functions while providing additional features for debugging, profiling, and optimization. It's particularly useful for game development, embedded systems, and performance-critical applications. 