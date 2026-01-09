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
// NOTE: Do NOT define __segment - it conflicts with libc++ internal __segment()
// functions. Original code using __segment is extremely rare.
// If needed, use 'unsigned short' directly.

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
//
// IMPORTANT: We only define _asm (single underscore), NOT __asm (double)
// because macOS system headers use __asm() for symbol aliasing.
// If the original code uses __asm, it will need manual editing.

// Option 1: Define as error to find all assembly blocks
// #define _asm _Static_assert(0, "Assembly block needs manual conversion")

// Option 2: Define as empty block (compiles but doesn't run assembly)
// WARNING: This will silently skip assembly code!
#ifndef _asm
#define _asm if(0)
#endif

// Note: We intentionally do NOT define __asm because it conflicts with
// __DARWIN_ALIAS() macros in macOS system headers. If original code uses
// __asm { } blocks, they must be manually converted or wrapped.

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
