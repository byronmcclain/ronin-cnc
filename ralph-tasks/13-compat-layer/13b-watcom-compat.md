# Task 13b: Watcom Compiler Compatibility

## Dependencies
- Task 13a (Windows Types) must be complete

## Context
The original Red Alert code was compiled with Watcom C++ 10.6, which has unique compiler-specific features not found in modern Clang. This includes:
- Non-standard calling conventions (__cdecl, __stdcall, etc.)
- Far/near pointer qualifiers (16-bit legacy)
- Pragma directives for compiler control
- Inline assembly syntax

This task creates a header that neutralizes all Watcom-specific syntax so the code compiles with Clang.

## Objective
Create `include/compat/watcom.h` that:
1. Defines/nullifies all Watcom calling conventions
2. Handles far/near pointer qualifiers
3. Manages pragma compatibility
4. Provides assembly transition markers (for later manual conversion)

## Technical Background

### Watcom Calling Conventions
```cpp
// Watcom uses these, Clang ignores them (they're x86-specific anyway)
__cdecl      // C calling convention (caller cleans stack)
__stdcall    // Windows standard (callee cleans stack)
__fastcall   // Register-based calling
__pascal     // Pascal convention (reverse param order)
__watcall    // Watcom-specific register convention
```

### Segment Qualifiers (16-bit Legacy)
```cpp
// These are meaningless in 32/64-bit flat memory model
__far        // Far pointer (segment:offset)
__near       // Near pointer (offset only)
__huge       // Huge pointer (normalized far)
__based(x)   // Based pointer (relative to segment)
```

### Watcom Pragmas
The original code uses various `#pragma` directives:
- `#pragma aux` - Modify calling conventions
- `#pragma library` - Link libraries
- `#pragma pack` - Structure packing (keep this!)
- `#pragma warning` - Warning control

### __WATCOMC__ Preprocessor Symbol
Some code checks `#ifdef __WATCOMC__` for conditional compilation. We may want to define this (with version 1100 for Watcom 11.0) or NOT define it depending on code paths.

## Deliverables
- [ ] `include/compat/watcom.h` - Watcom compatibility definitions
- [ ] Compiles without errors
- [ ] Verification script passes

## Files to Create

### include/compat/watcom.h (NEW)
```cpp
/**
 * Watcom C++ Compiler Compatibility Layer
 *
 * Neutralizes Watcom-specific syntax for compilation with modern Clang.
 * The original Red Alert code was compiled with Watcom C++ 10.6.
 *
 * Strategy:
 * - Define calling convention keywords as empty (no-op)
 * - Define segment qualifiers as empty (flat memory model)
 * - Suppress unknown pragma warnings
 * - Do NOT define __WATCOMC__ by default (prefer modern code paths)
 */

#ifndef COMPAT_WATCOM_H
#define COMPAT_WATCOM_H

// =============================================================================
// Compiler Detection
// =============================================================================

// We are NOT Watcom - use modern code paths when available
// Uncomment the following line ONLY if you need to force Watcom code paths:
// #define __WATCOMC__ 1100  // Watcom 11.0

// Clang/GCC detection
#if defined(__clang__)
#define COMPAT_COMPILER_CLANG 1
#elif defined(__GNUC__)
#define COMPAT_COMPILER_GCC 1
#endif

// =============================================================================
// Calling Convention Keywords
// =============================================================================
// These are x86-specific and have no meaning on ARM or in flat memory model.
// Define them as empty so the code compiles.

#ifndef __cdecl
#define __cdecl
#endif

#ifndef __stdcall
#define __stdcall
#endif

#ifndef __fastcall
#define __fastcall
#endif

#ifndef __pascal
#define __pascal
#endif

#ifndef __watcall
#define __watcall
#endif

#ifndef _cdecl
#define _cdecl
#endif

#ifndef _stdcall
#define _stdcall
#endif

#ifndef _fastcall
#define _fastcall
#endif

#ifndef _pascal
#define _pascal
#endif

// WINAPI/CALLBACK are typically __stdcall on Windows
#ifndef WINAPI
#define WINAPI
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef PASCAL
#define PASCAL
#endif

// =============================================================================
// Export/Import Keywords
// =============================================================================
// Used for DLL exports in Windows, not needed for static linking

#ifndef __export
#define __export
#endif

#ifndef __import
#define __import
#endif

#ifndef __declspec
#define __declspec(x)
#endif

#ifndef __dllexport
#define __dllexport
#endif

#ifndef __dllimport
#define __dllimport
#endif

// =============================================================================
// Segment Qualifiers (16-bit Legacy)
// =============================================================================
// These are completely meaningless in 32/64-bit flat memory model.
// Define as empty to allow compilation.

#ifndef __far
#define __far
#endif

#ifndef __near
#define __near
#endif

#ifndef __huge
#define __huge
#endif

#ifndef far
#define far
#endif

#ifndef near
#define near
#endif

#ifndef huge
#define huge
#endif

// __based pointers (relative to a segment)
// Format: type __based(segment) *ptr
// We can't easily handle this - define as nothing and hope for the best
#ifndef __based
#define __based(x)
#endif

// __segment type (segment selector)
#ifndef __segment
#define __segment unsigned short
#endif

// __segname (get segment by name)
#ifndef __segname
#define __segname(x) 0
#endif

// =============================================================================
// Function Modifiers
// =============================================================================

#ifndef __loadds
#define __loadds
#endif

#ifndef __saveregs
#define __saveregs
#endif

#ifndef __interrupt
#define __interrupt
#endif

// =============================================================================
// Inline Assembly
// =============================================================================
// Watcom uses: _asm { ... } or __asm { ... }
// Clang uses: __asm__("...")
//
// We CANNOT automatically convert assembly - it requires manual work.
// Define markers to help identify assembly blocks that need conversion.

// Option 1: Define as error to find all assembly blocks
// #define _asm _Static_assert(0, "Assembly block needs manual conversion")
// #define __asm _Static_assert(0, "Assembly block needs manual conversion")

// Option 2: Define as empty block (compiles but doesn't run assembly)
// WARNING: This will silently skip assembly code!
#ifndef _asm
#define _asm if(0)
#endif

#ifndef __asm
#define __asm if(0)
#endif

// emit keyword (inline opcodes)
#ifndef _emit
#define _emit(x)
#endif

// =============================================================================
// Pragma Handling
// =============================================================================
// Suppress warnings about unknown pragmas from Watcom code

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-value"
#pragma clang diagnostic ignored "-Wunused-variable"
// Allow #pragma once even though we use include guards
#pragma clang system_header
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

// Watcom's #pragma aux for customizing calling conventions
// Cannot be automatically converted - ignore silently
// Example: #pragma aux MyFunc parm [eax] [edx] value [eax];
// We handle this by not defining anything - unknown pragmas are ignored

// Note: #pragma pack IS supported by Clang and should work as-is
// Example: #pragma pack(push, 1) ... #pragma pack(pop)

// =============================================================================
// Memory Model Macros
// =============================================================================
// Watcom memory models (not relevant for 32-bit)

#ifndef M_I86SM  // Small model
#define M_I86SM 0
#endif

#ifndef M_I86MM  // Medium model
#define M_I86MM 0
#endif

#ifndef M_I86CM  // Compact model
#define M_I86CM 0
#endif

#ifndef M_I86LM  // Large model
#define M_I86LM 0
#endif

#ifndef M_I86HM  // Huge model
#define M_I86HM 0
#endif

#ifndef __FLAT__  // Flat (32-bit) model
#define __FLAT__ 1
#endif

// =============================================================================
// Miscellaneous Watcom-isms
// =============================================================================

// String/memory intrinsics
#ifndef _fstrcpy
#define _fstrcpy strcpy
#endif

#ifndef _fstrlen
#define _fstrlen strlen
#endif

#ifndef _fmemcpy
#define _fmemcpy memcpy
#endif

#ifndef _fmemset
#define _fmemset memset
#endif

// DOS-specific (not needed)
#ifndef MK_FP
#define MK_FP(seg, off) ((void*)(((unsigned long)(seg) << 4) + (off)))
#endif

#ifndef FP_SEG
#define FP_SEG(ptr) (0)
#endif

#ifndef FP_OFF
#define FP_OFF(ptr) ((unsigned)(uintptr_t)(ptr))
#endif

// =============================================================================
// Suppress Specific Warnings
// =============================================================================
// The original code may have patterns that generate warnings in modern compilers

#if defined(__clang__)
// Suppress warnings commonly triggered by legacy code
#pragma clang diagnostic ignored "-Wparentheses"
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#pragma clang diagnostic ignored "-Wbitwise-op-parentheses"
#pragma clang diagnostic ignored "-Wshift-op-parentheses"
#pragma clang diagnostic ignored "-Wdangling-else"
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wchar-subscripts"
#pragma clang diagnostic ignored "-Wformat"
#pragma clang diagnostic ignored "-Wformat-security"
// Keep important warnings enabled
// #pragma clang diagnostic warning "-Wreturn-type"
// #pragma clang diagnostic warning "-Wuninitialized"
#endif

// =============================================================================
// End of Watcom Compatibility Header
// =============================================================================

#endif // COMPAT_WATCOM_H
```

## Verification Command
```bash
ralph-tasks/verify/check-watcom-compat.sh
```

## Verification Script

### ralph-tasks/verify/check-watcom-compat.sh (NEW)
```bash
#!/bin/bash
# Verification script for Watcom compatibility header

set -e

SCRIPT_DIR="$(dirname "$0")"
ROOT_DIR="$SCRIPT_DIR/../.."

echo "=== Checking Watcom Compatibility Header ==="
echo ""

cd "$ROOT_DIR"

# Step 1: Check header exists
echo "[1/4] Checking header file exists..."
if [ ! -f "include/compat/watcom.h" ]; then
    echo "VERIFY_FAILED: include/compat/watcom.h not found"
    exit 1
fi
echo "  OK: Header file exists"

# Step 2: Syntax check
echo "[2/4] Checking syntax..."
if ! clang++ -std=c++17 -fsyntax-only -I include include/compat/watcom.h 2>&1; then
    echo "VERIFY_FAILED: Header has syntax errors"
    exit 1
fi
echo "  OK: Syntax valid"

# Step 3: Compile test with Watcom-style code
echo "[3/4] Compiling test program with Watcom syntax..."
cat > /tmp/test_watcom.cpp << 'EOF'
#include "compat/watcom.h"
#include <cstdio>
#include <cstring>

// Test calling conventions (should compile as no-op)
int __cdecl cdecl_func(int a) { return a; }
int __stdcall stdcall_func(int a) { return a; }
int __fastcall fastcall_func(int a) { return a; }
int WINAPI winapi_func(int a) { return a; }

// Test segment qualifiers (should compile as no-op)
char __far* far_ptr = nullptr;
char __near* near_ptr = nullptr;
char far* far_ptr2 = nullptr;
char near* near_ptr2 = nullptr;

// Test export keywords (should compile as no-op)
__export void exported_func() {}

// Test inline assembly marker (should skip silently)
void asm_test() {
    int x = 5;
    _asm {
        // This block should be skipped entirely
        mov eax, 1
        add eax, 2
    }
    printf("Assembly test: x = %d\n", x);
}

// Test far string functions
void string_test() {
    char buf[64];
    _fstrcpy(buf, "Hello");
    size_t len = _fstrlen(buf);
    printf("String test: len = %zu\n", len);
}

int main() {
    printf("Testing Watcom compatibility...\n\n");

    // Test calling conventions
    if (cdecl_func(1) != 1) return 1;
    if (stdcall_func(2) != 2) return 1;
    if (fastcall_func(3) != 3) return 1;
    if (winapi_func(4) != 4) return 1;
    printf("Calling conventions: OK\n");

    // Test pointers
    far_ptr = nullptr;
    near_ptr = nullptr;
    printf("Segment qualifiers: OK\n");

    // Test assembly skip
    asm_test();
    printf("Assembly skip: OK\n");

    // Test string functions
    string_test();

    // Test flat model macro
    if (!__FLAT__) return 1;
    printf("Flat model: OK\n");

    printf("\nAll Watcom compatibility tests passed!\n");
    return 0;
}
EOF

if ! clang++ -std=c++17 -I include /tmp/test_watcom.cpp -o /tmp/test_watcom 2>&1; then
    echo "VERIFY_FAILED: Test program compilation failed"
    rm -f /tmp/test_watcom.cpp
    exit 1
fi
echo "  OK: Test program compiled"

# Step 4: Run test
echo "[4/4] Running test program..."
if ! /tmp/test_watcom; then
    echo "VERIFY_FAILED: Test program failed"
    rm -f /tmp/test_watcom.cpp /tmp/test_watcom
    exit 1
fi

# Cleanup
rm -f /tmp/test_watcom.cpp /tmp/test_watcom

echo ""
echo "VERIFY_SUCCESS"
exit 0
```

## Implementation Steps
1. Create `include/compat/watcom.h` with the content above
2. Make verification script executable: `chmod +x ralph-tasks/verify/check-watcom-compat.sh`
3. Run verification script
4. Fix any issues and repeat

## Success Criteria
- Header compiles without errors
- Calling convention keywords compile as no-ops
- Segment qualifiers compile as no-ops
- Inline assembly blocks are safely skipped
- Far string functions redirect to standard functions
- Test program runs successfully

## Common Issues

### "use of undeclared identifier" for assembly
- The `_asm { }` block should compile as `if(0) { }` which skips contents
- If actual assembly opcodes cause parse errors, the block structure may be wrong
- May need to handle `__asm` differently based on actual usage

### Conflicts with system headers
- Include watcom.h BEFORE any system headers
- Some macros may conflict with standard library definitions

### #pragma pack not working
- `#pragma pack` IS supported by Clang - don't override it
- Verify packed structures have correct sizes

### Warning flood from legacy code
- The header suppresses many common warnings
- Enable specific warnings if needed for debugging

## Completion Promise
When verification passes, output:
```
<promise>TASK_13B_COMPLETE</promise>
```

## Escape Hatch
If stuck after 12 iterations:
- Document what's blocking in `ralph-tasks/blocked/13b.md`
- List attempted approaches
- Output: `<promise>TASK_13B_BLOCKED</promise>`

## Max Iterations
12
