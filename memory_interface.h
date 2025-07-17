/**************************************************************************/
/*  memory_interface.h                                                   */
/**************************************************************************/
/*  Independent Memory Management Module                                  */
/*  User-friendly interface layer                                        */
/**************************************************************************/

#pragma once

#include "platform_defines.h"
#include "error_handling.h"
#include "memory_manager.h"
#include <new>
#include <type_traits>

// Memory management macros (compatible with Godot)
#define memalloc(m_size) Memory::alloc_static(m_size)
#define memalloc_zeroed(m_size) Memory::alloc_static_zeroed(m_size)
#define memrealloc(m_mem, m_size) Memory::realloc_static(m_mem, m_size)
#define memfree(m_mem) Memory::free_static(m_mem)

// Post-initialization handler (can be customized)
MEMORY_ALWAYS_INLINE void postinitialize_handler(void*) {
    // Default implementation does nothing
    // Can be overridden by user code
}

// Pre-deletion handler (can be customized)
MEMORY_ALWAYS_INLINE bool predelete_handler(void*) {
    // Default implementation allows deletion
    // Can be overridden by user code to prevent deletion
    return true;
}

// Post-initialize helper
template<typename T>
MEMORY_ALWAYS_INLINE T* _post_initialize(T* p_obj) {
    postinitialize_handler(p_obj);
    return p_obj;
}

// Object creation macros
#define memnew(m_class) _post_initialize(::new ("") m_class)

#define memnew_allocator(m_class, m_allocator) \
    _post_initialize(::new (m_allocator::alloc) m_class)

#define memnew_placement(m_placement, m_class) \
    _post_initialize(::new (m_placement) m_class)

// Object deletion functions
template<typename T>
void memdelete(T* p_class) {
    if (!p_class) {
        return;
    }
    
    if (!predelete_handler(p_class)) {
        return; // Handler decided not to delete
    }
    
    if constexpr (!std::is_trivially_destructible_v<T>) {
        p_class->~T();
    }
    
    Memory::free_static(p_class, false);
}

template<typename T, typename A>
void memdelete_allocator(T* p_class) {
    if (!p_class) {
        return;
    }
    
    if (!predelete_handler(p_class)) {
        return; // Handler decided not to delete
    }
    
    if constexpr (!std::is_trivially_destructible_v<T>) {
        p_class->~T();
    }
    
    A::free(p_class);
}

// Safe deletion macro
#define memdelete_notnull(m_v) \
    do { \
        if (m_v) { \
            memdelete(m_v); \
            m_v = nullptr; \
        } \
    } while (0)

// Array allocation and deallocation
#define memnew_arr(m_class, m_count) memnew_arr_template<m_class>(m_count)

// Helper to get element count pointer
MEMORY_FORCE_INLINE memory_uint64_t* _get_element_count_ptr(memory_uint8_t* p_ptr) {
    return reinterpret_cast<memory_uint64_t*>(p_ptr - Memory::DATA_OFFSET + Memory::ELEMENT_OFFSET);
}

// Array allocation template
template<typename T>
T* memnew_arr_template(memory_size_t p_elements) {
    if (p_elements == 0) {
        return nullptr;
    }
    
    // Calculate total size needed
    memory_size_t len = sizeof(T) * p_elements;
    memory_uint8_t* mem = static_cast<memory_uint8_t*>(Memory::alloc_static(len, true));
    
    T* failptr = nullptr;
    MEMORY_ERR_FAIL_NULL_V(mem, failptr);
    
    // Store element count
    memory_uint64_t* elem_count_ptr = _get_element_count_ptr(mem);
    *elem_count_ptr = p_elements;
    
    // Construct elements if needed
    if constexpr (!std::is_trivially_constructible_v<T>) {
        T* elems = reinterpret_cast<T*>(mem);
        
        for (memory_size_t i = 0; i < p_elements; i++) {
            ::new (&elems[i]) T;
        }
    }
    
    return reinterpret_cast<T*>(mem);
}

// Fast array placement construction
template<typename T>
MEMORY_FORCE_INLINE void memnew_arr_placement(T* p_start, memory_size_t p_num) {
    // Check if we can use memset optimization
    if constexpr (std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>) {
        // Check if zero-initialization is equivalent to default construction
        if constexpr (std::is_arithmetic_v<T> || std::is_pointer_v<T>) {
            std::memset(static_cast<void*>(p_start), 0, p_num * sizeof(T));
            return;
        }
    }
    
    // Fallback to individual construction
    for (memory_size_t i = 0; i < p_num; i++) {
        memnew_placement(p_start + i, T);
    }
}

// Get array length
template<typename T>
memory_size_t memarr_len(const T* p_class) {
    if (!p_class) {
        return 0;
    }
    
    memory_uint8_t* ptr = reinterpret_cast<memory_uint8_t*>(const_cast<T*>(p_class));
    memory_uint64_t* elem_count_ptr = _get_element_count_ptr(ptr);
    return static_cast<memory_size_t>(*elem_count_ptr);
}

// Array deletion
template<typename T>
void memdelete_arr(T* p_class) {
    if (!p_class) {
        return;
    }
    
    memory_uint8_t* ptr = reinterpret_cast<memory_uint8_t*>(p_class);
    
    // Destruct elements if needed
    if constexpr (!std::is_trivially_destructible_v<T>) {
        memory_uint64_t* elem_count_ptr = _get_element_count_ptr(ptr);
        memory_uint64_t elem_count = *elem_count_ptr;
        
        for (memory_uint64_t i = 0; i < elem_count; i++) {
            p_class[i].~T();
        }
    }
    
    Memory::free_static(ptr, true);
}

// Safe array deletion macro
#define memdelete_arr_notnull(m_v) \
    do { \
        if (m_v) { \
            memdelete_arr(m_v); \
            m_v = nullptr; \
        } \
    } while (0)

// Modern C++ style interface
namespace memory {
    // RAII wrapper for memory management
    template<typename T>
    class unique_ptr {
    private:
        T* ptr_;
        
    public:
        unique_ptr() : ptr_(nullptr) {}
        
        explicit unique_ptr(T* p) : ptr_(p) {}
        
        template<typename... Args>
        static unique_ptr make(Args&&... args) {
            T* ptr = _post_initialize(::new ("") T(std::forward<Args>(args)...));
            return unique_ptr(ptr);
        }
        
        ~unique_ptr() {
            if (ptr_) {
                memdelete(ptr_);
            }
        }
        
        // Move constructor
        unique_ptr(unique_ptr&& other) noexcept : ptr_(other.ptr_) {
            other.ptr_ = nullptr;
        }
        
        // Move assignment
        unique_ptr& operator=(unique_ptr&& other) noexcept {
            if (this != &other) {
                if (ptr_) {
                    memdelete(ptr_);
                }
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }
        
        // Delete copy constructor and assignment
        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;
        
        // Access operators
        T* operator->() const { return ptr_; }
        T& operator*() const { return *ptr_; }
        T* get() const { return ptr_; }
        
        // Release ownership
        T* release() {
            T* tmp = ptr_;
            ptr_ = nullptr;
            return tmp;
        }
        
        // Reset pointer
        void reset(T* p = nullptr) {
            if (ptr_) {
                memdelete(ptr_);
            }
            ptr_ = p;
        }
        
        // Boolean conversion
        explicit operator bool() const { return ptr_ != nullptr; }
    };
    
    // Array wrapper
    template<typename T>
    class unique_array {
    private:
        T* ptr_;
        memory_size_t size_;
        
    public:
        unique_array() : ptr_(nullptr), size_(0) {}
        
        explicit unique_array(memory_size_t count) : size_(count) {
            ptr_ = memnew_arr_template<T>(count);
        }
        
        ~unique_array() {
            if (ptr_) {
                memdelete_arr(ptr_);
            }
        }
        
        // Move constructor
        unique_array(unique_array&& other) noexcept : ptr_(other.ptr_), size_(other.size_) {
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        
        // Move assignment
        unique_array& operator=(unique_array&& other) noexcept {
            if (this != &other) {
                if (ptr_) {
                    memdelete_arr(ptr_);
                }
                ptr_ = other.ptr_;
                size_ = other.size_;
                other.ptr_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }
        
        // Delete copy constructor and assignment
        unique_array(const unique_array&) = delete;
        unique_array& operator=(const unique_array&) = delete;
        
        // Access operators
        T& operator[](memory_size_t index) const {
            MEMORY_ERR_FAIL_INDEX_V(index, size_, ptr_[0]);
            return ptr_[index];
        }
        
        T* get() const { return ptr_; }
        memory_size_t size() const { return size_; }
        
        // Release ownership
        T* release() {
            T* tmp = ptr_;
            ptr_ = nullptr;
            size_ = 0;
            return tmp;
        }
        
        // Reset array
        void reset(memory_size_t count = 0) {
            if (ptr_) {
                memdelete_arr(ptr_);
            }
            if (count > 0) {
                ptr_ = memnew_arr_template<T>(count);
                size_ = count;
            } else {
                ptr_ = nullptr;
                size_ = 0;
            }
        }
        
        // Boolean conversion
        explicit operator bool() const { return ptr_ != nullptr; }
        
        // Iterator support
        T* begin() const { return ptr_; }
        T* end() const { return ptr_ + size_; }
    };
    
    // Factory functions
    template<typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args) {
        return unique_ptr<T>::make(std::forward<Args>(args)...);
    }
    
    template<typename T>
    unique_array<T> make_unique_array(memory_size_t count) {
        return unique_array<T>(count);
    }
    
    // Memory statistics functions
    inline memory_uint64_t get_usage() {
        return Memory::get_mem_usage();
    }
    
    inline memory_uint64_t get_peak_usage() {
        return Memory::get_mem_max_usage();
    }
    
    inline memory_uint64_t get_available() {
        return Memory::get_mem_available();
    }
    
    inline MemoryStats get_stats() {
        return Memory::get_memory_stats();
    }
    
    inline void reset_stats() {
        Memory::reset_memory_stats();
    }
    
    inline void dump_allocations() {
        Memory::dump_memory_allocations();
    }
    
    // Runtime configuration
    inline void set_error_handler(MemoryErrorHandler handler) {
        set_memory_error_handler(handler);
    }
    
    inline MemoryErrorHandler get_error_handler() {
        return get_memory_error_handler();
    }
    
    inline MemoryRuntimeConfig& get_runtime_config() {
        return MemoryRuntimeConfig::instance();
    }
}

// Global nil object (similar to Godot's implementation)
struct _GlobalNil {
    int color = 1;
    _GlobalNil* right = nullptr;
    _GlobalNil* left = nullptr;
    _GlobalNil* parent = nullptr;
    
    _GlobalNil() {
        left = this;
        right = this;
        parent = this;
    }
};

struct _GlobalNilClass {
    static _GlobalNil _nil;
};

inline _GlobalNil _GlobalNilClass::_nil;

// Compatibility aliases for different memory managers
namespace fast_memory {
    using Memory = FastMemory;
    
    #define fast_memalloc(m_size) FastMemory::alloc_static(m_size)
    #define fast_memfree(m_mem) FastMemory::free_static(m_mem)
    
    template<typename T>
    void fast_memdelete(T* p_class) {
        if (!p_class) return;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            p_class->~T();
        }
        FastMemory::free_static(p_class, false);
    }
}

namespace debug_memory {
    using Memory = DebugMemory;
    
    #define debug_memalloc(m_size) DebugMemory::alloc_static(m_size)
    #define debug_memfree(m_mem) DebugMemory::free_static(m_mem)
    
    template<typename T>
    void debug_memdelete(T* p_class) {
        if (!p_class) return;
        if (!predelete_handler(p_class)) return;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            p_class->~T();
        }
        DebugMemory::free_static(p_class, false);
    }
}

namespace embedded_memory {
    using Memory = EmbeddedMemory;
    
    #define embedded_memalloc(m_size) EmbeddedMemory::alloc_static(m_size)
    #define embedded_memfree(m_mem) EmbeddedMemory::free_static(m_mem)
    
    template<typename T>
    void embedded_memdelete(T* p_class) {
        if (!p_class) return;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            p_class->~T();
        }
        EmbeddedMemory::free_static(p_class, false);
    }
} 