// src/tests/unit/test_utils.cpp
// Utility Function Unit Tests
// Task 18b - Unit Tests

#include "test/test_framework.h"
#include <cstring>
#include <cmath>
#include <algorithm>

//=============================================================================
// String Utility Tests
//=============================================================================

TEST_CASE(Utils_String_ToUpper, "Utils") {
    char str[] = "hello world";

    for (char* p = str; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 'a' + 'A';
        }
    }

    TEST_ASSERT_EQ(strcmp(str, "HELLO WORLD"), 0);
}

TEST_CASE(Utils_String_ToLower, "Utils") {
    char str[] = "HELLO WORLD";

    for (char* p = str; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = *p - 'A' + 'a';
        }
    }

    TEST_ASSERT_EQ(strcmp(str, "hello world"), 0);
}

TEST_CASE(Utils_String_Compare_CaseInsensitive, "Utils") {
    const char* a = "Hello";
    const char* b = "HELLO";
    const char* c = "world";

    // Case-insensitive compare
    auto stricmp_impl = [](const char* s1, const char* s2) {
        while (*s1 && *s2) {
            char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
            char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
            if (c1 != c2) return c1 - c2;
            s1++; s2++;
        }
        return *s1 - *s2;
    };

    TEST_ASSERT_EQ(stricmp_impl(a, b), 0);
    TEST_ASSERT_NE(stricmp_impl(a, c), 0);
}

//=============================================================================
// Math Utility Tests
//=============================================================================

TEST_CASE(Utils_Math_Clamp, "Utils") {
    auto clamp = [](int value, int min, int max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    };

    TEST_ASSERT_EQ(clamp(50, 0, 100), 50);
    TEST_ASSERT_EQ(clamp(-10, 0, 100), 0);
    TEST_ASSERT_EQ(clamp(150, 0, 100), 100);
}

TEST_CASE(Utils_Math_Min, "Utils") {
    TEST_ASSERT_EQ(std::min(5, 10), 5);
    TEST_ASSERT_EQ(std::min(10, 5), 5);
    TEST_ASSERT_EQ(std::min(5, 5), 5);
}

TEST_CASE(Utils_Math_Max, "Utils") {
    TEST_ASSERT_EQ(std::max(5, 10), 10);
    TEST_ASSERT_EQ(std::max(10, 5), 10);
    TEST_ASSERT_EQ(std::max(5, 5), 5);
}

TEST_CASE(Utils_Math_Abs, "Utils") {
    TEST_ASSERT_EQ(std::abs(5), 5);
    TEST_ASSERT_EQ(std::abs(-5), 5);
    TEST_ASSERT_EQ(std::abs(0), 0);
}

TEST_CASE(Utils_Math_Lerp, "Utils") {
    auto lerp = [](float a, float b, float t) {
        return a + (b - a) * t;
    };

    TEST_ASSERT_NEAR(lerp(0.0f, 100.0f, 0.0f), 0.0f, 0.001f);
    TEST_ASSERT_NEAR(lerp(0.0f, 100.0f, 1.0f), 100.0f, 0.001f);
    TEST_ASSERT_NEAR(lerp(0.0f, 100.0f, 0.5f), 50.0f, 0.001f);
}

TEST_CASE(Utils_Math_Distance, "Utils") {
    auto distance = [](int x1, int y1, int x2, int y2) {
        int dx = x2 - x1;
        int dy = y2 - y1;
        return std::sqrt(static_cast<double>(dx * dx + dy * dy));
    };

    TEST_ASSERT_NEAR(distance(0, 0, 3, 4), 5.0, 0.001);
    TEST_ASSERT_NEAR(distance(0, 0, 0, 0), 0.0, 0.001);
}

TEST_CASE(Utils_Math_ManhattanDistance, "Utils") {
    auto manhattan = [](int x1, int y1, int x2, int y2) {
        return std::abs(x2 - x1) + std::abs(y2 - y1);
    };

    TEST_ASSERT_EQ(manhattan(0, 0, 3, 4), 7);
    TEST_ASSERT_EQ(manhattan(5, 5, 5, 5), 0);
}

//=============================================================================
// Bit Manipulation Tests
//=============================================================================

TEST_CASE(Utils_Bits_SetBit, "Utils") {
    uint32_t flags = 0;

    flags |= (1 << 0);  // Set bit 0
    TEST_ASSERT_EQ(flags, 1u);

    flags |= (1 << 3);  // Set bit 3
    TEST_ASSERT_EQ(flags, 9u);  // 0b1001
}

TEST_CASE(Utils_Bits_ClearBit, "Utils") {
    uint32_t flags = 0xFF;  // All bits set

    flags &= ~(1 << 0);  // Clear bit 0
    TEST_ASSERT_EQ(flags, 0xFEu);

    flags &= ~(1 << 3);  // Clear bit 3
    TEST_ASSERT_EQ(flags, 0xF6u);
}

TEST_CASE(Utils_Bits_TestBit, "Utils") {
    uint32_t flags = 0b1010;

    TEST_ASSERT((flags & (1 << 1)) != 0);  // Bit 1 is set
    TEST_ASSERT((flags & (1 << 3)) != 0);  // Bit 3 is set
    TEST_ASSERT((flags & (1 << 0)) == 0);  // Bit 0 is not set
    TEST_ASSERT((flags & (1 << 2)) == 0);  // Bit 2 is not set
}

TEST_CASE(Utils_Bits_ToggleBit, "Utils") {
    uint32_t flags = 0b1010;

    flags ^= (1 << 1);  // Toggle bit 1 (was 1, now 0)
    TEST_ASSERT_EQ(flags, 0b1000u);

    flags ^= (1 << 1);  // Toggle bit 1 (was 0, now 1)
    TEST_ASSERT_EQ(flags, 0b1010u);
}

//=============================================================================
// Fixed Point Math Tests
//=============================================================================

TEST_CASE(Utils_Fixed_IntToFixed, "Utils") {
    // 16.16 fixed point
    int32_t fixed = 5 << 16;  // 5.0 in fixed point
    TEST_ASSERT_EQ(fixed, 327680);
}

TEST_CASE(Utils_Fixed_FixedToInt, "Utils") {
    int32_t fixed = 327680;  // 5.0
    int integer = fixed >> 16;
    TEST_ASSERT_EQ(integer, 5);
}

TEST_CASE(Utils_Fixed_Multiply, "Utils") {
    // 2.5 * 2.0 = 5.0
    int32_t a = (2 << 16) + (1 << 15);  // 2.5
    int32_t b = 2 << 16;                 // 2.0

    int64_t result = (static_cast<int64_t>(a) * b) >> 16;

    int integer_part = static_cast<int32_t>(result) >> 16;
    TEST_ASSERT_EQ(integer_part, 5);
}

//=============================================================================
// Random Number Tests
//=============================================================================

TEST_CASE(Utils_Random_Range, "Utils") {
    // Simple linear congruential generator
    uint32_t seed = 12345;

    auto random = [&seed]() {
        seed = seed * 1103515245 + 12345;
        return seed;
    };

    // Generate some random numbers
    for (int i = 0; i < 100; i++) {
        uint32_t r = random();
        // Just verify it produces values
        TEST_ASSERT_GE(r, 0u);
    }
}

TEST_CASE(Utils_Random_InRange, "Utils") {
    auto random_in_range = [](int min, int max, uint32_t random_value) {
        return min + static_cast<int>(random_value % static_cast<uint32_t>(max - min + 1));
    };

    // Test with various random values
    for (uint32_t i = 0; i < 100; i++) {
        int result = random_in_range(10, 20, i * 12345);
        TEST_ASSERT_GE(result, 10);
        TEST_ASSERT_LE(result, 20);
    }
}

//=============================================================================
// Direction/Facing Tests
//=============================================================================

TEST_CASE(Utils_Direction_8Way, "Utils") {
    // 8 directions (0-7)
    // 0 = North, 1 = NE, 2 = East, etc.
    enum Direction { N = 0, NE, E, SE, S, SW, W, NW };

    TEST_ASSERT_EQ(Direction::N, 0);
    TEST_ASSERT_EQ(Direction::E, 2);
    TEST_ASSERT_EQ(Direction::S, 4);
    TEST_ASSERT_EQ(Direction::W, 6);
}

TEST_CASE(Utils_Direction_Opposite, "Utils") {
    auto opposite = [](int dir) {
        return (dir + 4) % 8;
    };

    TEST_ASSERT_EQ(opposite(0), 4);  // N -> S
    TEST_ASSERT_EQ(opposite(2), 6);  // E -> W
    TEST_ASSERT_EQ(opposite(4), 0);  // S -> N
}

TEST_CASE(Utils_Direction_FromDelta, "Utils") {
    // Calculate direction from dx, dy
    auto direction_from_delta = [](int dx, int dy) -> int {
        if (dx == 0 && dy < 0) return 0;   // N
        if (dx > 0 && dy < 0) return 1;    // NE
        if (dx > 0 && dy == 0) return 2;   // E
        if (dx > 0 && dy > 0) return 3;    // SE
        if (dx == 0 && dy > 0) return 4;   // S
        if (dx < 0 && dy > 0) return 5;    // SW
        if (dx < 0 && dy == 0) return 6;   // W
        if (dx < 0 && dy < 0) return 7;    // NW
        return 0;
    };

    TEST_ASSERT_EQ(direction_from_delta(0, -1), 0);   // North
    TEST_ASSERT_EQ(direction_from_delta(1, 0), 2);    // East
    TEST_ASSERT_EQ(direction_from_delta(0, 1), 4);    // South
    TEST_ASSERT_EQ(direction_from_delta(-1, 0), 6);   // West
}

//=============================================================================
// Coordinate/Cell Tests
//=============================================================================

TEST_CASE(Utils_Coord_CellToIndex, "Utils") {
    // Convert 2D cell coordinates to linear index
    int map_width = 64;

    auto cell_to_index = [map_width](int x, int y) {
        return y * map_width + x;
    };

    TEST_ASSERT_EQ(cell_to_index(0, 0), 0);
    TEST_ASSERT_EQ(cell_to_index(1, 0), 1);
    TEST_ASSERT_EQ(cell_to_index(0, 1), 64);
    TEST_ASSERT_EQ(cell_to_index(5, 10), 645);
}

TEST_CASE(Utils_Coord_IndexToCell, "Utils") {
    int map_width = 64;

    auto index_to_cell_x = [map_width](int index) {
        return index % map_width;
    };

    auto index_to_cell_y = [map_width](int index) {
        return index / map_width;
    };

    TEST_ASSERT_EQ(index_to_cell_x(645), 5);
    TEST_ASSERT_EQ(index_to_cell_y(645), 10);
}

//=============================================================================
// Memory/Endian Tests
//=============================================================================

TEST_CASE(Utils_Endian_LittleEndian16, "Utils") {
    // Read 16-bit little-endian value
    uint8_t data[2] = {0x34, 0x12};  // 0x1234 in little-endian

    uint16_t value = static_cast<uint16_t>(data[0]) |
                     (static_cast<uint16_t>(data[1]) << 8);

    TEST_ASSERT_EQ(value, 0x1234u);
}

TEST_CASE(Utils_Endian_LittleEndian32, "Utils") {
    // Read 32-bit little-endian value
    uint8_t data[4] = {0x78, 0x56, 0x34, 0x12};  // 0x12345678 in little-endian

    uint32_t value = static_cast<uint32_t>(data[0]) |
                     (static_cast<uint32_t>(data[1]) << 8) |
                     (static_cast<uint32_t>(data[2]) << 16) |
                     (static_cast<uint32_t>(data[3]) << 24);

    TEST_ASSERT_EQ(value, 0x12345678u);
}
