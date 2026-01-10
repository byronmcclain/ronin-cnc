// src/platform/memory_arena.h
// Frame Arena Allocator
// Task 18h - Performance Optimization

#ifndef MEMORY_ARENA_H
#define MEMORY_ARENA_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <utility>

//=============================================================================
// Linear Arena Allocator
//=============================================================================

class MemoryArena {
public:
    explicit MemoryArena(size_t initial_size = 1024 * 1024)  // 1MB default
        : block_size_(initial_size) {
        allocate_block(initial_size);
    }

    ~MemoryArena() {
        for (char* block : blocks_) {
            delete[] block;
        }
    }

    // Allocate raw memory (aligned)
    void* allocate(size_t size, size_t alignment = 8) {
        // Align current position
        size_t aligned_pos = (current_pos_ + alignment - 1) & ~(alignment - 1);

        if (aligned_pos + size > current_block_size_) {
            // Need new block
            size_t new_block_size = block_size_;
            if (size > new_block_size) {
                new_block_size = size + alignment;
            }
            allocate_block(new_block_size);
            aligned_pos = 0;
        }

        void* ptr = current_block_ + aligned_pos;
        current_pos_ = aligned_pos + size;
        total_allocated_ += size;

        return ptr;
    }

    // Allocate and construct
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    // Allocate array
    template<typename T>
    T* create_array(size_t count) {
        void* mem = allocate(sizeof(T) * count, alignof(T));
        T* arr = static_cast<T*>(mem);
        for (size_t i = 0; i < count; i++) {
            new (&arr[i]) T();
        }
        return arr;
    }

    // Reset arena (reuse memory)
    void reset() {
        // Keep first block, release others
        while (blocks_.size() > 1) {
            delete[] blocks_.back();
            blocks_.pop_back();
        }

        if (!blocks_.empty()) {
            current_block_ = blocks_[0];
            current_block_size_ = block_size_;
        }
        current_pos_ = 0;
        total_allocated_ = 0;
    }

    // Statistics
    size_t get_total_allocated() const { return total_allocated_; }
    size_t get_total_capacity() const {
        size_t total = 0;
        for (size_t i = 0; i < blocks_.size(); i++) {
            total += (i == 0) ? block_size_ : block_size_;
        }
        return total;
    }
    size_t get_block_count() const { return blocks_.size(); }

    // Scoped marker for temporary allocations
    class Marker {
    public:
        Marker(MemoryArena& arena)
            : arena_(arena)
            , block_index_(arena.blocks_.size() - 1)
            , position_(arena.current_pos_) {}

        ~Marker() {
            // Restore arena state
            while (arena_.blocks_.size() > block_index_ + 1) {
                delete[] arena_.blocks_.back();
                arena_.blocks_.pop_back();
            }
            arena_.current_block_ = arena_.blocks_[block_index_];
            arena_.current_pos_ = position_;
        }

    private:
        MemoryArena& arena_;
        size_t block_index_;
        size_t position_;
    };

    Marker create_marker() { return Marker(*this); }

private:
    void allocate_block(size_t size) {
        char* block = new char[size];
        blocks_.push_back(block);
        current_block_ = block;
        current_block_size_ = size;
        current_pos_ = 0;
    }

    std::vector<char*> blocks_;
    char* current_block_ = nullptr;
    size_t current_block_size_ = 0;
    size_t current_pos_ = 0;
    size_t block_size_;
    size_t total_allocated_ = 0;
};

//=============================================================================
// Frame Allocator (reset each frame)
//=============================================================================

class FrameAllocator {
public:
    static FrameAllocator& instance() {
        static FrameAllocator instance;
        return instance;
    }

    void begin_frame() {
        arena_.reset();
    }

    void end_frame() {
        // Could add stats collection here
    }

    void* allocate(size_t size, size_t alignment = 8) {
        return arena_.allocate(size, alignment);
    }

    template<typename T, typename... Args>
    T* create(Args&&... args) {
        return arena_.create<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* create_array(size_t count) {
        return arena_.create_array<T>(count);
    }

    size_t get_frame_allocation() const {
        return arena_.get_total_allocated();
    }

private:
    FrameAllocator() : arena_(2 * 1024 * 1024) {}  // 2MB frame arena
    MemoryArena arena_;
};

//=============================================================================
// Convenience Macros
//=============================================================================

#define FRAME_ALLOC(size) FrameAllocator::instance().allocate(size)
#define FRAME_NEW(Type, ...) FrameAllocator::instance().create<Type>(__VA_ARGS__)
#define FRAME_ARRAY(Type, count) FrameAllocator::instance().create_array<Type>(count)

#endif // MEMORY_ARENA_H
