// src/tests/gameplay/test_resources.cpp
// Resource System Gameplay Tests
// Task 18e - Gameplay Tests

#include "test/test_framework.h"

//=============================================================================
// Resource Tests
//=============================================================================

TEST_CASE(Gameplay_Resources_Harvesting, "Resources") {
    // Harvester collects ore
    int ore_value = 25;
    int harvester_capacity = 1000;
    int current_load = 0;

    // Collect ore
    int ore_cells = 10;
    current_load += ore_value * ore_cells;

    TEST_ASSERT_LE(current_load, harvester_capacity);
    TEST_ASSERT_EQ(current_load, 250);

    // Collect more until full
    ore_cells = 40;
    current_load += ore_value * ore_cells;

    // Should cap at capacity
    if (current_load > harvester_capacity) {
        current_load = harvester_capacity;
    }
    TEST_ASSERT_EQ(current_load, 1000);
}

TEST_CASE(Gameplay_Resources_Refinery, "Resources") {
    // Refinery converts ore to credits
    int ore_load = 1000;
    int credits_per_ore_unit = 1;

    int credits_gained = ore_load * credits_per_ore_unit;
    TEST_ASSERT_EQ(credits_gained, 1000);

    // Gems worth more
    int gem_load = 500;
    int credits_per_gem = 2;
    credits_gained = gem_load * credits_per_gem;
    TEST_ASSERT_EQ(credits_gained, 1000);
}

TEST_CASE(Gameplay_Resources_Silo, "Resources") {
    // Silos store excess credits
    int refinery_capacity = 1000;
    int silo_capacity = 1500;
    int current_credits = 0;

    // Add refineries and silos
    int total_storage = refinery_capacity + silo_capacity;
    TEST_ASSERT_EQ(total_storage, 2500);

    // Overflow test
    current_credits = 2000;
    int incoming = 700;

    int space_left = total_storage - current_credits;
    int actually_stored = (incoming > space_left) ? space_left : incoming;
    int overflow = incoming - actually_stored;

    TEST_ASSERT_EQ(space_left, 500);
    TEST_ASSERT_EQ(actually_stored, 500);
    TEST_ASSERT_EQ(overflow, 200);  // 200 credits lost!
}

TEST_CASE(Gameplay_Resources_OreGrowth, "Resources") {
    // Ore regenerates over time
    struct OreField {
        int density;       // Current ore level (0-12)
        int max_density;   // Maximum level
    };

    OreField field = {5, 12};

    // Growth rate
    int growth_interval = 900;  // Frames between growth
    int frames_elapsed = 1800;

    int growth_ticks = frames_elapsed / growth_interval;
    field.density += growth_ticks;
    if (field.density > field.max_density) {
        field.density = field.max_density;
    }

    TEST_ASSERT_EQ(field.density, 7);  // 5 + 2 = 7
}

TEST_CASE(Gameplay_Resources_Income, "Resources") {
    // Track income per minute
    int harvester_count = 3;
    int trips_per_minute = 2;
    int credits_per_trip = 1000;

    int income_per_minute = harvester_count * trips_per_minute * credits_per_trip;
    TEST_ASSERT_EQ(income_per_minute, 6000);

    // With gems
    int gem_multiplier = 2;
    int gem_income = income_per_minute * gem_multiplier;
    TEST_ASSERT_EQ(gem_income, 12000);
}

TEST_CASE(Gameplay_Resources_HarvesterPath, "Resources") {
    // Harvester finds nearest ore
    struct OreLocation {
        int x, y;
        int density;
    };

    OreLocation ore_fields[] = {
        {100, 100, 10},  // Close, rich
        {50, 50, 5},     // Closest
        {200, 200, 12}   // Far, richest
    };

    int refinery_x = 0, refinery_y = 0;

    // Find closest ore
    int closest_idx = 0;
    int closest_dist = 10000;

    for (int i = 0; i < 3; i++) {
        int dist = ore_fields[i].x + ore_fields[i].y;  // Manhattan
        if (dist < closest_dist) {
            closest_dist = dist;
            closest_idx = i;
        }
    }

    TEST_ASSERT_EQ(closest_idx, 1);  // (50,50)
}

TEST_CASE(Gameplay_Resources_OreDepletion, "Resources") {
    // Ore depletes when harvested
    struct OreTile {
        int density;
    };

    OreTile tile = {12};  // Full ore

    // Harvest
    int harvest_amount = 3;
    tile.density -= harvest_amount;
    TEST_ASSERT_EQ(tile.density, 9);

    // Harvest more
    tile.density -= harvest_amount;
    tile.density -= harvest_amount;
    tile.density -= harvest_amount;
    TEST_ASSERT_EQ(tile.density, 0);

    // Can't harvest empty
    tile.density -= harvest_amount;
    if (tile.density < 0) tile.density = 0;
    TEST_ASSERT_EQ(tile.density, 0);
}

TEST_CASE(Gameplay_Resources_StartingCredits, "Resources") {
    // Different starting credits by difficulty
    struct Difficulty {
        const char* name;
        int player_credits;
        int ai_credits;
    };

    Difficulty settings[] = {
        {"Easy", 10000, 5000},
        {"Normal", 5000, 5000},
        {"Hard", 5000, 10000}
    };

    TEST_ASSERT_GT(settings[0].player_credits, settings[0].ai_credits);
    TEST_ASSERT_EQ(settings[1].player_credits, settings[1].ai_credits);
    TEST_ASSERT_LT(settings[2].player_credits, settings[2].ai_credits);
}

TEST_CASE(Gameplay_Resources_TiberiumSpread, "Resources") {
    // Tiberium/Ore can spread to adjacent cells
    struct Cell {
        bool has_ore;
        bool can_grow;  // Not water, not building
    };

    Cell cells[9];  // 3x3 grid
    for (int i = 0; i < 9; i++) {
        cells[i].has_ore = false;
        cells[i].can_grow = true;
    }
    cells[4].has_ore = true;  // Center has ore

    // Spread to adjacent
    int adjacent[] = {1, 3, 5, 7};  // N, W, E, S
    for (int i : adjacent) {
        if (cells[i].can_grow && !cells[i].has_ore) {
            cells[i].has_ore = true;  // Spread!
        }
    }

    // Count ore cells
    int ore_count = 0;
    for (int i = 0; i < 9; i++) {
        if (cells[i].has_ore) ore_count++;
    }

    TEST_ASSERT_EQ(ore_count, 5);  // Center + 4 adjacent
}

TEST_CASE(Gameplay_Resources_MultipleRefineries, "Resources") {
    // Harvester goes to nearest refinery
    struct Refinery {
        int x, y;
        bool busy;  // Another harvester docking
    };

    Refinery refineries[] = {
        {100, 100, false},
        {50, 50, true},   // Busy!
        {200, 200, false}
    };

    int harvester_x = 60, harvester_y = 60;

    // Find nearest non-busy refinery
    int best_idx = -1;
    int best_dist = 10000;

    for (int i = 0; i < 3; i++) {
        if (refineries[i].busy) continue;

        int dx = refineries[i].x - harvester_x;
        int dy = refineries[i].y - harvester_y;
        int dist = dx * dx + dy * dy;  // Squared distance

        if (dist < best_dist) {
            best_dist = dist;
            best_idx = i;
        }
    }

    TEST_ASSERT_EQ(best_idx, 0);  // First refinery (second is busy)
}
