# Task 06d: Memory Integration Test

## Dependencies
- Task 06c must be complete (FFI Exports)

## Context
This task verifies the complete memory system works correctly from C++ code. It tests allocation, freeing, reallocation, memory operations, and statistics tracking.

## Objective
Add memory tests to `src/main.cpp` that verify:
1. `Platform_Alloc()` returns valid pointers
2. `Platform_Free()` doesn't crash
3. `Platform_Realloc()` works correctly
4. Memory operations (copy, set, compare) work
5. Statistics tracking is accurate
6. All tests pass

## Deliverables
- [ ] `test_memory()` function in `src/main.cpp`
- [ ] Updated `run_tests()` to call memory tests
- [ ] All tests pass
- [ ] Verification passes

## Files to Modify

### src/main.cpp (MODIFY)
Add test function after `test_timing()`:
```cpp
int test_memory() {
    TEST("Memory system");

    // Test basic allocation
    void* ptr1 = Platform_Alloc(1024, 0);  // MEM_NORMAL
    if (ptr1 == nullptr) FAIL("Allocation returned null");

    // Test zero-initialized allocation
    void* ptr2 = Platform_Alloc(256, 0x0004);  // MEM_CLEAR
    if (ptr2 == nullptr) FAIL("Clear allocation returned null");

    // Verify zero-initialized
    uint8_t* bytes = static_cast<uint8_t*>(ptr2);
    bool all_zero = true;
    for (int i = 0; i < 256; i++) {
        if (bytes[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (!all_zero) FAIL("MEM_CLEAR did not zero memory");

    // Test memory operations
    const char* test_str = "Hello, Memory!";
    size_t str_len = strlen(test_str) + 1;
    Platform_MemCopy(ptr1, test_str, str_len);

    if (Platform_MemCmp(ptr1, test_str, str_len) != 0) {
        FAIL("MemCopy/MemCmp failed");
    }

    // Test MemSet
    Platform_MemSet(ptr2, 0xAB, 256);
    bytes = static_cast<uint8_t*>(ptr2);
    bool all_ab = true;
    for (int i = 0; i < 256; i++) {
        if (bytes[i] != 0xAB) {
            all_ab = false;
            break;
        }
    }
    if (!all_ab) FAIL("MemSet failed");

    // Test ZeroMemory
    Platform_ZeroMemory(ptr2, 256);
    bytes = static_cast<uint8_t*>(ptr2);
    all_zero = true;
    for (int i = 0; i < 256; i++) {
        if (bytes[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (!all_zero) FAIL("ZeroMemory failed");

    // Test reallocation
    void* ptr3 = Platform_Alloc(64, 0);
    if (ptr3 == nullptr) FAIL("Small allocation failed");

    void* ptr3_new = Platform_Realloc(ptr3, 64, 128);
    if (ptr3_new == nullptr) FAIL("Realloc failed");

    // Check statistics
    size_t allocated = Platform_Mem_GetAllocated();
    size_t count = Platform_Mem_GetCount();
    size_t peak = Platform_Mem_GetPeak();

    if (allocated == 0) FAIL("Allocated should be > 0");
    if (count == 0) FAIL("Count should be > 0");
    if (peak == 0) FAIL("Peak should be > 0");

    printf("(alloc=%zu, count=%zu, peak=%zu) ", allocated, count, peak);

    // Free all allocations
    Platform_Free(ptr1, 1024);
    Platform_Free(ptr2, 256);
    Platform_Free(ptr3_new, 128);

    // Check RAM queries don't crash
    size_t free_ram = Platform_Ram_Free();
    size_t total_ram = Platform_Ram_Total();
    if (free_ram == 0 || total_ram == 0) {
        printf("(ram: free=%zu, total=%zu) ", free_ram, total_ram);
    }

    PASS();
    return 0;
}
```

### src/main.cpp (MODIFY)
Update `run_tests()` to include memory test:
```cpp
// After timing test, add:
{
    Platform_Init();
    failures += test_memory();
    Platform_Mem_DumpLeaks();  // Should show no leaks
    Platform_Shutdown();
}
```

## Verification Command
```bash
cd /Users/ooartist/Src/Ronin_CnC && \
cmake --build build 2>&1 | tail -5 && \
./build/RedAlert --test 2>&1 | grep -q "All tests passed" && \
echo "VERIFY_SUCCESS"
```

## Implementation Steps
1. Add `test_memory()` function to main.cpp
2. Test basic allocation and MEM_CLEAR flag
3. Test memory operations (MemCopy, MemSet, MemCmp, ZeroMemory)
4. Test reallocation
5. Test statistics functions
6. Add memory test call to `run_tests()`
7. Add `Platform_Mem_DumpLeaks()` call after test
8. Build and run tests

## Completion Promise
When verification passes, output:
```
<promise>TASK_06D_COMPLETE</promise>
```

Then proceed to **Phase 07: File System**.

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/06d.md`
- List attempted approaches
- Output: `<promise>TASK_06D_BLOCKED</promise>`

## Max Iterations
12
