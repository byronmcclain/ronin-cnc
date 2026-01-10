// src/tests/visual/screenshot_utils.h
// Screenshot capture and comparison utilities
// Task 18d - Visual Tests

#ifndef SCREENSHOT_UTILS_H
#define SCREENSHOT_UTILS_H

#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include "test/test_framework.h"

//=============================================================================
// Screenshot Capture
//=============================================================================

/// Capture current backbuffer to file
bool Screenshot_Capture(const char* filename);

/// Capture to memory buffer
bool Screenshot_CaptureToBuffer(std::vector<uint8_t>& buffer,
                                 int& width, int& height);

//=============================================================================
// Image Comparison
//=============================================================================

struct ImageCompareResult {
    bool match;              // Within threshold
    int total_pixels;        // Total pixels compared
    int different_pixels;    // Pixels exceeding threshold
    float difference_ratio;  // Ratio of different pixels
    int max_difference;      // Maximum single-pixel difference
};

/// Compare two image buffers
ImageCompareResult Screenshot_Compare(
    const uint8_t* a, const uint8_t* b,
    int width, int height,
    int threshold = 5);      // Per-pixel threshold

/// Compare captured buffer against reference file
ImageCompareResult Screenshot_CompareToReference(
    const char* reference_file,
    int threshold = 5);

//=============================================================================
// Reference Image Management
//=============================================================================

/// Generate reference image from current frame
bool Screenshot_SaveReference(const char* name);

/// Load reference image
bool Screenshot_LoadReference(const char* name,
                               std::vector<uint8_t>& buffer,
                               int& width, int& height);

/// Get reference image path
std::string Screenshot_GetReferencePath(const char* name);

//=============================================================================
// Visual Test Macros
//=============================================================================

#define VISUAL_ASSERT_MATCHES_REFERENCE(name) \
    do { \
        auto result = Screenshot_CompareToReference(name); \
        if (!result.match) { \
            std::ostringstream _ss; \
            _ss << "Visual mismatch: " << (result.difference_ratio * 100) \
                << "% different (max diff: " << result.max_difference << ")"; \
            throw TestAssertionFailed(_ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

#define VISUAL_GENERATE_REFERENCE(name) \
    Screenshot_SaveReference(name)

#endif // SCREENSHOT_UTILS_H
