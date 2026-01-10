/**
 * Standard Color Remap Tables Implementation
 */

#include "game/graphics/remap_tables.h"
#include "platform.h"
#include <cstring>

// =============================================================================
// Table Storage
// =============================================================================

static bool s_tables_initialized = false;

// Identity table (no change)
static uint8_t s_identity_table[256];

// House color remap tables (8 houses * 256 colors)
static uint8_t s_house_tables[8][256];

// Shadow table
static uint8_t s_shadow_table[256];

// Fade tables (16 levels * 256 colors)
static uint8_t s_fade_tables[16 * 256];

// Ghost table
static uint8_t s_ghost_table[256];

// =============================================================================
// Color Matching
// =============================================================================

// Find closest palette color to given RGB
static uint8_t FindClosestColor(const uint8_t* palette, int r, int g, int b) {
    int best_index = 0;
    int best_dist = 0x7FFFFFFF;

    for (int i = 1; i < 256; i++) {  // Skip index 0 (transparent)
        int pr = palette[i * 3 + 0];
        int pg = palette[i * 3 + 1];
        int pb = palette[i * 3 + 2];

        // Simple RGB distance (could use LAB for better results)
        int dr = r - pr;
        int dg = g - pg;
        int db = b - pb;
        int dist = dr * dr + dg * dg + db * db;

        if (dist < best_dist) {
            best_dist = dist;
            best_index = i;
        }
    }

    return static_cast<uint8_t>(best_index);
}

// =============================================================================
// House Colors
// =============================================================================

// Base house color RGB values (primary)
static const int HOUSE_COLORS_PRIMARY[8][3] = {
    {255, 255, 0},    // GOLD (Yellow)
    {0, 160, 255},    // LTBLUE (Light Blue)
    {255, 0, 0},      // RED
    {0, 200, 0},      // GREEN
    {255, 128, 0},    // ORANGE
    {128, 128, 128},  // GREY
    {0, 0, 255},      // BLUE
    {140, 80, 40},    // BROWN
};

// Remap range for house colors (inclusive)
static const int HOUSE_REMAP_START = 80;
static const int HOUSE_REMAP_END = 95;
static const int HOUSE_REMAP_COUNT = HOUSE_REMAP_END - HOUSE_REMAP_START + 1;

static void GenerateHouseTable(int house, const uint8_t* palette, uint8_t* output) {
    // Start with identity
    for (int i = 0; i < 256; i++) {
        output[i] = static_cast<uint8_t>(i);
    }

    // Get house base color
    int base_r = HOUSE_COLORS_PRIMARY[house][0];
    int base_g = HOUSE_COLORS_PRIMARY[house][1];
    int base_b = HOUSE_COLORS_PRIMARY[house][2];

    // Remap the house color range
    for (int i = 0; i < HOUSE_REMAP_COUNT; i++) {
        int src_index = HOUSE_REMAP_START + i;

        // Calculate shade factor based on position in range
        // Index 0 = darkest, Index 15 = lightest
        float shade = static_cast<float>(i) / (HOUSE_REMAP_COUNT - 1);

        // Interpolate from dark to full color
        int r = static_cast<int>(base_r * (0.3f + 0.7f * shade));
        int g = static_cast<int>(base_g * (0.3f + 0.7f * shade));
        int b = static_cast<int>(base_b * (0.3f + 0.7f * shade));

        // Clamp
        r = r > 255 ? 255 : (r < 0 ? 0 : r);
        g = g > 255 ? 255 : (g < 0 ? 0 : g);
        b = b > 255 ? 255 : (b < 0 ? 0 : b);

        // Find closest palette color
        output[src_index] = FindClosestColor(palette, r, g, b);
    }
}

const uint8_t* GetHouseRemapTable(int house) {
    if (house < 0 || house >= HOUSE_COLOR_COUNT) {
        return s_identity_table;
    }
    return s_house_tables[house];
}

const uint8_t* GetPlayerRemapTable(int player, bool is_multiplayer) {
    if (!is_multiplayer) {
        // Single player: 0 = Allied (Gold), 1 = Soviet (Red)
        if (player == 0) return s_house_tables[HOUSE_COLOR_GOLD];
        if (player == 1) return s_house_tables[HOUSE_COLOR_RED];
        return s_identity_table;
    }

    // Multiplayer: direct mapping
    if (player < 0 || player >= HOUSE_COLOR_COUNT) {
        return s_identity_table;
    }
    return s_house_tables[player];
}

// =============================================================================
// Shadow Tables
// =============================================================================

void GenerateShadowTable(const uint8_t* palette, uint8_t* output, float intensity) {
    // Clamp intensity
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;

    float factor = 1.0f - intensity;

    for (int i = 0; i < 256; i++) {
        if (i == 0) {
            output[i] = 0;  // Transparent stays transparent
            continue;
        }

        // Get original color
        int r = palette[i * 3 + 0];
        int g = palette[i * 3 + 1];
        int b = palette[i * 3 + 2];

        // Darken
        r = static_cast<int>(r * factor);
        g = static_cast<int>(g * factor);
        b = static_cast<int>(b * factor);

        // Find closest palette color
        output[i] = FindClosestColor(palette, r, g, b);
    }
}

const uint8_t* GetShadowTable() {
    return s_shadow_table;
}

// =============================================================================
// Fade Tables
// =============================================================================

void GenerateFadeTables(const uint8_t* palette, uint8_t* output) {
    for (int level = 0; level < 16; level++) {
        uint8_t* table = output + level * 256;

        // Factor ranges from 1.0 (level 0) to 0.0 (level 15)
        float factor = 1.0f - (static_cast<float>(level) / 15.0f);

        for (int i = 0; i < 256; i++) {
            if (i == 0) {
                table[i] = 0;  // Transparent stays transparent
                continue;
            }

            // Get original color
            int r = palette[i * 3 + 0];
            int g = palette[i * 3 + 1];
            int b = palette[i * 3 + 2];

            // Fade towards black
            r = static_cast<int>(r * factor);
            g = static_cast<int>(g * factor);
            b = static_cast<int>(b * factor);

            // Find closest palette color
            table[i] = FindClosestColor(palette, r, g, b);
        }
    }
}

const uint8_t* GetFadeTables() {
    return s_fade_tables;
}

// =============================================================================
// Ghost Table
// =============================================================================

static void GenerateGhostTable(const uint8_t* palette, uint8_t* output) {
    // Ghost effect lightens colors
    for (int i = 0; i < 256; i++) {
        if (i == 0) {
            output[i] = 0;
            continue;
        }

        // Get original color
        int r = palette[i * 3 + 0];
        int g = palette[i * 3 + 1];
        int b = palette[i * 3 + 2];

        // Lighten by 50%
        r = r + (255 - r) / 2;
        g = g + (255 - g) / 2;
        b = b + (255 - b) / 2;

        output[i] = FindClosestColor(palette, r, g, b);
    }
}

const uint8_t* GetGhostTable() {
    return s_ghost_table;
}

// =============================================================================
// Identity Table
// =============================================================================

const uint8_t* GetIdentityTable() {
    return s_identity_table;
}

// =============================================================================
// Initialization
// =============================================================================

void InitRemapTables(const uint8_t* palette) {
    if (!palette) {
        return;
    }

    // Initialize identity table
    for (int i = 0; i < 256; i++) {
        s_identity_table[i] = static_cast<uint8_t>(i);
    }

    // Generate house color tables
    for (int h = 0; h < HOUSE_COLOR_COUNT; h++) {
        GenerateHouseTable(h, palette, s_house_tables[h]);
    }

    // Generate shadow table (50% darkening)
    GenerateShadowTable(palette, s_shadow_table, 0.5f);

    // Generate fade tables
    GenerateFadeTables(palette, s_fade_tables);

    // Generate ghost table
    GenerateGhostTable(palette, s_ghost_table);

    s_tables_initialized = true;
}

bool AreRemapTablesInitialized() {
    return s_tables_initialized;
}
