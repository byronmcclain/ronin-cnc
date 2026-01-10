// src/platform/memory_pool.cpp
// Object Pool Implementation
// Task 18h - Performance Optimization

#include "platform/memory_pool.h"

// Template implementations are in the header
// This file exists for any non-template utilities

//=============================================================================
// Pool Statistics Helper
//=============================================================================

struct PoolStats {
    size_t allocated;
    size_t free;
    size_t total;
    size_t memory_bytes;
};

// Would collect stats from multiple pools if needed
