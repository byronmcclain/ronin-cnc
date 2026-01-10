// src/platform/memory_pool.h
// Object Pool Memory Management
// Task 18h - Performance Optimization

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <utility>

//=============================================================================
// Fixed-Size Object Pool
//=============================================================================

template<typename T, size_t BlockSize = 64>
class ObjectPool {
public:
    ObjectPool() {
        allocate_block();
    }

    ~ObjectPool() {
        for (char* block : blocks_) {
            delete[] block;
        }
    }

    // Allocate an object (default constructed)
    T* allocate() {
        if (free_list_.empty()) {
            allocate_block();
        }

        T* obj = free_list_.back();
        free_list_.pop_back();
        new (obj) T();  // Placement new
        allocated_count_++;
        return obj;
    }

    // Allocate with constructor arguments
    template<typename... Args>
    T* allocate(Args&&... args) {
        if (free_list_.empty()) {
            allocate_block();
        }

        T* obj = free_list_.back();
        free_list_.pop_back();
        new (obj) T(std::forward<Args>(args)...);
        allocated_count_++;
        return obj;
    }

    // Return object to pool
    void deallocate(T* obj) {
        if (obj == nullptr) return;

        obj->~T();  // Call destructor
        free_list_.push_back(obj);
        allocated_count_--;
    }

    // Statistics
    size_t get_allocated_count() const { return allocated_count_; }
    size_t get_free_count() const { return free_list_.size(); }
    size_t get_total_capacity() const { return blocks_.size() * BlockSize; }

    // Clear all
    void clear() {
        free_list_.clear();
        for (char* block : blocks_) {
            T* objects = reinterpret_cast<T*>(block);
            for (size_t i = 0; i < BlockSize; i++) {
                free_list_.push_back(&objects[i]);
            }
        }
        allocated_count_ = 0;
    }

private:
    void allocate_block() {
        char* block = new char[sizeof(T) * BlockSize];
        blocks_.push_back(block);

        T* objects = reinterpret_cast<T*>(block);
        for (size_t i = 0; i < BlockSize; i++) {
            free_list_.push_back(&objects[i]);
        }
    }

    std::vector<char*> blocks_;
    std::vector<T*> free_list_;
    size_t allocated_count_ = 0;
};

//=============================================================================
// Pool Handle (for safer pool access)
//=============================================================================

template<typename T>
class PoolHandle {
public:
    PoolHandle() : pool_(nullptr), obj_(nullptr) {}
    PoolHandle(ObjectPool<T>* pool, T* obj) : pool_(pool), obj_(obj) {}

    ~PoolHandle() {
        release();
    }

    // Move semantics
    PoolHandle(PoolHandle&& other) noexcept
        : pool_(other.pool_), obj_(other.obj_) {
        other.pool_ = nullptr;
        other.obj_ = nullptr;
    }

    PoolHandle& operator=(PoolHandle&& other) noexcept {
        if (this != &other) {
            release();
            pool_ = other.pool_;
            obj_ = other.obj_;
            other.pool_ = nullptr;
            other.obj_ = nullptr;
        }
        return *this;
    }

    // No copying
    PoolHandle(const PoolHandle&) = delete;
    PoolHandle& operator=(const PoolHandle&) = delete;

    T* get() const { return obj_; }
    T* operator->() const { return obj_; }
    T& operator*() const { return *obj_; }
    explicit operator bool() const { return obj_ != nullptr; }

    void release() {
        if (pool_ && obj_) {
            pool_->deallocate(obj_);
        }
        pool_ = nullptr;
        obj_ = nullptr;
    }

private:
    ObjectPool<T>* pool_;
    T* obj_;
};

#endif // MEMORY_POOL_H
