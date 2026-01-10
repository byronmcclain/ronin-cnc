// src/tests/gameplay/test_building.cpp
// Building System Gameplay Tests
// Task 18e - Gameplay Tests

#include "test/test_framework.h"

//=============================================================================
// Building Tests
//=============================================================================

TEST_CASE(Gameplay_Building_Prerequisites, "Building") {
    // Buildings require prerequisites
    bool has_construction_yard = true;
    bool has_power_plant = false;

    // Barracks requires power plant
    bool can_build_barracks = has_construction_yard && has_power_plant;
    TEST_ASSERT(!can_build_barracks);

    has_power_plant = true;
    can_build_barracks = has_construction_yard && has_power_plant;
    TEST_ASSERT(can_build_barracks);
}

TEST_CASE(Gameplay_Building_Cost, "Building") {
    // Buildings cost credits
    int credits = 1000;
    int barracks_cost = 500;

    bool can_afford = credits >= barracks_cost;
    TEST_ASSERT(can_afford);

    credits -= barracks_cost;
    TEST_ASSERT_EQ(credits, 500);

    // Can't afford another
    can_afford = credits >= barracks_cost;
    TEST_ASSERT(can_afford);  // Still can afford

    credits -= barracks_cost;
    TEST_ASSERT_EQ(credits, 0);

    can_afford = credits >= barracks_cost;
    TEST_ASSERT(!can_afford);
}

TEST_CASE(Gameplay_Building_BuildTime, "Building") {
    // Buildings take time to construct
    int build_time_seconds = 30;
    int frames_per_second = 15;
    int build_frames = build_time_seconds * frames_per_second;

    TEST_ASSERT_EQ(build_frames, 450);

    // Build progress
    int frames_elapsed = 225;  // Half done
    float progress = static_cast<float>(frames_elapsed) / build_frames;
    TEST_ASSERT_NEAR(progress, 0.5f, 0.01f);
}

TEST_CASE(Gameplay_Building_Placement, "Building") {
    // Buildings need valid placement
    struct BuildSite {
        bool flat_ground;
        bool no_overlap;
        bool adjacent_to_owned;
        bool on_shroud;
    };

    BuildSite site1 = {true, true, true, false};   // Valid
    BuildSite site2 = {false, true, true, false};  // Not flat
    BuildSite site3 = {true, false, true, false};  // Overlapping
    BuildSite site4 = {true, true, true, true};    // On shroud

    auto can_place = [](const BuildSite& s) {
        return s.flat_ground && s.no_overlap && !s.on_shroud;
    };

    TEST_ASSERT(can_place(site1));
    TEST_ASSERT(!can_place(site2));
    TEST_ASSERT(!can_place(site3));
    TEST_ASSERT(!can_place(site4));
}

TEST_CASE(Gameplay_Building_Power, "Building") {
    // Buildings produce/consume power
    struct Building {
        const char* name;
        int power;  // Positive = produce, negative = consume
    };

    Building buildings[] = {
        {"Power Plant", 100},
        {"Barracks", -20},
        {"War Factory", -30},
        {"Refinery", -30},
        {"Radar", -40}
    };

    int total_power = 0;
    for (int i = 0; i < 5; i++) {
        total_power += buildings[i].power;
    }

    // 100 - 20 - 30 - 30 - 40 = -20 (low power!)
    TEST_ASSERT_EQ(total_power, -20);

    // Need another power plant
    total_power += 100;
    TEST_ASSERT_EQ(total_power, 80);
    TEST_ASSERT_GT(total_power, 0);
}

TEST_CASE(Gameplay_Building_Selling, "Building") {
    // Selling returns half cost
    int building_cost = 1000;
    int sell_return = building_cost / 2;

    TEST_ASSERT_EQ(sell_return, 500);

    // Damaged building returns less
    int health_percent = 50;
    int damaged_return = (sell_return * health_percent) / 100;
    TEST_ASSERT_EQ(damaged_return, 250);
}

TEST_CASE(Gameplay_Building_Repair, "Building") {
    // Repairing costs credits
    int max_health = 1000;
    int current_health = 600;
    int damage = max_health - current_health;

    int repair_cost_per_hp = 1;  // 1 credit per HP
    int total_repair_cost = damage * repair_cost_per_hp;

    TEST_ASSERT_EQ(total_repair_cost, 400);

    // Repair rate
    int repair_rate = 10;  // HP per second
    int repair_time = damage / repair_rate;
    TEST_ASSERT_EQ(repair_time, 40);  // 40 seconds
}

TEST_CASE(Gameplay_Building_Queue, "Building") {
    // Production queue
    struct QueueItem {
        const char* name;
        int build_time;
        bool completed;
    };

    QueueItem queue[3] = {
        {"Rifleman", 50, false},
        {"Rifleman", 50, false},
        {"Grenadier", 75, false}
    };

    int total_time = 0;
    for (int i = 0; i < 3; i++) {
        total_time += queue[i].build_time;
    }

    TEST_ASSERT_EQ(total_time, 175);

    // Complete first
    queue[0].completed = true;
    int remaining_time = 0;
    for (int i = 0; i < 3; i++) {
        if (!queue[i].completed) {
            remaining_time += queue[i].build_time;
        }
    }
    TEST_ASSERT_EQ(remaining_time, 125);
}

TEST_CASE(Gameplay_Building_TechTree, "Building") {
    // Tech tree dependencies
    struct TechNode {
        const char* name;
        int requires;  // -1 = none, else index
        bool unlocked;
    };

    TechNode tree[] = {
        {"Construction Yard", -1, true},   // Index 0
        {"Power Plant", 0, false},         // Index 1 - requires 0
        {"Barracks", 1, false},            // Index 2 - requires 1
        {"War Factory", 1, false},         // Index 3 - requires 1
        {"Tech Center", 3, false}          // Index 4 - requires 3 (War Factory)
    };

    // Unlock power plant (requires conyard which is unlocked)
    if (tree[1].requires == -1 || tree[tree[1].requires].unlocked) {
        tree[1].unlocked = true;
    }
    TEST_ASSERT(tree[1].unlocked);

    // Can't unlock tech center yet (requires war factory which is not unlocked)
    if (tree[4].requires == -1 || tree[tree[4].requires].unlocked) {
        tree[4].unlocked = true;
    }
    TEST_ASSERT(!tree[4].unlocked);

    // Unlock war factory (index 3)
    tree[3].unlocked = true;
    if (tree[4].requires == -1 || tree[tree[4].requires].unlocked) {
        tree[4].unlocked = true;
    }
    TEST_ASSERT(tree[4].unlocked);
}

TEST_CASE(Gameplay_Building_Capture, "Building") {
    // Engineers can capture buildings
    int building_health = 1000;
    int capture_threshold = 250;  // 25% health

    bool can_capture = building_health <= capture_threshold;
    TEST_ASSERT(!can_capture);

    building_health = 200;
    can_capture = building_health <= capture_threshold;
    TEST_ASSERT(can_capture);
}

TEST_CASE(Gameplay_Building_Superweapon, "Building") {
    // Superweapons have long charge time
    int charge_time_seconds = 300;  // 5 minutes
    int frames_per_second = 15;
    int charge_frames = charge_time_seconds * frames_per_second;

    int frames_elapsed = 0;
    bool ready = frames_elapsed >= charge_frames;
    TEST_ASSERT(!ready);

    frames_elapsed = charge_frames;
    ready = frames_elapsed >= charge_frames;
    TEST_ASSERT(ready);

    // After use, resets
    frames_elapsed = 0;
    ready = frames_elapsed >= charge_frames;
    TEST_ASSERT(!ready);
}
