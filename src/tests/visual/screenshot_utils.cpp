// src/tests/visual/screenshot_utils.cpp
// Screenshot utilities implementation
// Task 18d - Visual Tests

#include "screenshot_utils.h"
#include "platform.h"
#include <fstream>
#include <cstring>
#include <algorithm>

// Simple BMP file writing (no external dependencies)
static bool WriteBMP(const char* filename, const uint8_t* data,
                     int width, int height, const PaletteEntry* palette) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // BMP header
    uint8_t header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    // File size (filled later)
    // Reserved (0)
    // Offset to pixel data (header + palette) - use low byte
    *reinterpret_cast<uint32_t*>(&header[10]) = 54 + 256 * 4;
    header[14] = 40;            // DIB header size
    // Width (filled later)
    // Height (filled later)
    header[26] = 1;             // Planes
    header[28] = 8;             // Bits per pixel
    // Compression = 0
    // Image size = 0
    // Pixels per meter = 0
    header[46] = 0;             // Colors used (0 = 256)
    header[47] = 1;

    // Fill in dimensions
    *reinterpret_cast<int32_t*>(&header[18]) = width;
    *reinterpret_cast<int32_t*>(&header[22]) = height;

    // Row padding (BMP rows must be multiple of 4 bytes)
    int row_padding = (4 - (width % 4)) % 4;
    int row_size = width + row_padding;
    int image_size = row_size * height;

    // File size
    *reinterpret_cast<uint32_t*>(&header[2]) = 54 + 1024 + image_size;

    file.write(reinterpret_cast<char*>(header), 54);

    // Write palette (BGRA format)
    for (int i = 0; i < 256; i++) {
        uint8_t bgra[4];
        if (palette) {
            bgra[0] = palette[i].b;
            bgra[1] = palette[i].g;
            bgra[2] = palette[i].r;
            bgra[3] = 0;
        } else {
            // Default grayscale
            bgra[0] = static_cast<uint8_t>(i);
            bgra[1] = static_cast<uint8_t>(i);
            bgra[2] = static_cast<uint8_t>(i);
            bgra[3] = 0;
        }
        file.write(reinterpret_cast<char*>(bgra), 4);
    }

    // Write pixel data (bottom-up)
    std::vector<uint8_t> row(row_size, 0);
    for (int y = height - 1; y >= 0; y--) {
        memcpy(row.data(), data + y * width, width);
        file.write(reinterpret_cast<char*>(row.data()), row_size);
    }

    return true;
}

bool Screenshot_Capture(const char* filename) {
    uint8_t* buffer;
    int32_t w, h, p;

    if (Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p) != 0) {
        return false;
    }

    // Get current palette
    PaletteEntry palette[256];
    Platform_Graphics_GetPalette(palette, 0, 256);

    // Copy to contiguous buffer if pitch != width
    std::vector<uint8_t> contiguous;
    const uint8_t* src = buffer;

    if (p != w) {
        contiguous.resize(w * h);
        for (int y = 0; y < h; y++) {
            memcpy(contiguous.data() + y * w, buffer + y * p, w);
        }
        src = contiguous.data();
    }

    return WriteBMP(filename, src, w, h, palette);
}

bool Screenshot_CaptureToBuffer(std::vector<uint8_t>& out_buffer,
                                 int& width, int& height) {
    uint8_t* buffer;
    int32_t w, h, p;

    if (Platform_Graphics_GetBackBuffer(&buffer, &w, &h, &p) != 0) {
        return false;
    }

    width = w;
    height = h;
    out_buffer.resize(w * h);

    for (int y = 0; y < h; y++) {
        memcpy(out_buffer.data() + y * w, buffer + y * p, w);
    }

    return true;
}

ImageCompareResult Screenshot_Compare(const uint8_t* a, const uint8_t* b,
                                       int width, int height, int threshold) {
    ImageCompareResult result = {};
    result.total_pixels = width * height;

    for (int i = 0; i < result.total_pixels; i++) {
        int diff = std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i]));
        result.max_difference = std::max(result.max_difference, diff);

        if (diff > threshold) {
            result.different_pixels++;
        }
    }

    result.difference_ratio =
        static_cast<float>(result.different_pixels) / result.total_pixels;
    result.match = result.difference_ratio < 0.01f;  // < 1% different

    return result;
}

std::string Screenshot_GetReferencePath(const char* name) {
    return std::string("reference/") + name + ".bmp";
}

bool Screenshot_SaveReference(const char* name) {
    std::string path = Screenshot_GetReferencePath(name);
    return Screenshot_Capture(path.c_str());
}

ImageCompareResult Screenshot_CompareToReference(const char* reference_file,
                                                  int threshold) {
    ImageCompareResult result = {};
    result.match = false;

    // Capture current
    std::vector<uint8_t> current;
    int cur_w, cur_h;
    if (!Screenshot_CaptureToBuffer(current, cur_w, cur_h)) {
        return result;
    }

    // Load reference
    std::vector<uint8_t> reference;
    int ref_w, ref_h;
    if (!Screenshot_LoadReference(reference_file, reference, ref_w, ref_h)) {
        return result;
    }

    // Size mismatch
    if (cur_w != ref_w || cur_h != ref_h) {
        result.different_pixels = cur_w * cur_h;
        result.total_pixels = cur_w * cur_h;
        result.difference_ratio = 1.0f;
        return result;
    }

    return Screenshot_Compare(current.data(), reference.data(),
                              cur_w, cur_h, threshold);
}

bool Screenshot_LoadReference(const char* name,
                               std::vector<uint8_t>& buffer,
                               int& width, int& height) {
    std::string path = Screenshot_GetReferencePath(name);

    // Simple BMP loading
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    // Read header
    uint8_t header[54];
    file.read(reinterpret_cast<char*>(header), 54);

    if (header[0] != 'B' || header[1] != 'M') {
        return false;
    }

    width = *reinterpret_cast<int32_t*>(&header[18]);
    height = *reinterpret_cast<int32_t*>(&header[22]);

    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        return false;
    }

    // Skip palette
    file.seekg(54 + 1024);

    // Read pixel data
    int row_padding = (4 - (width % 4)) % 4;
    buffer.resize(width * height);

    for (int y = height - 1; y >= 0; y--) {
        file.read(reinterpret_cast<char*>(buffer.data() + y * width), width);
        file.seekg(row_padding, std::ios::cur);
    }

    return true;
}
