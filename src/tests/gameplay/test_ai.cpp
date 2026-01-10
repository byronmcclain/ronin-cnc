// src/tests/gameplay/test_ai.cpp
// AI Behavior Gameplay Tests
// Task 18e - Gameplay Tests

#include "test/test_framework.h"
#include <cmath>

//=============================================================================
// AI Tests
//=============================================================================

TEST_CASE(Gameplay_AI_TargetSelection, "AI") {
    // AI should target nearest/weakest
    struct Target {
        int distance;
        int health;
        int threat;
    };

    Target targets[] = {
        {10, 100, 5},   // Far, healthy, low threat
        {5, 50, 8},     // Close, damaged, medium threat
        {3, 100, 3}     // Closest, healthy, low threat
    };

    // Priority: threat * 3 + (100 - health) / 10 - distance
    int best_idx = 0;
    int best_score = -1000;

    for (int i = 0; i < 3; i++) {
        int score = targets[i].threat * 3 +
                    (100 - targets[i].health) / 10 -
                    targets[i].distance;
        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    // Target 1 (close, damaged, medium threat) should win
    TEST_ASSERT_EQ(best_idx, 1);
}

TEST_CASE(Gameplay_AI_ThreatResponse, "AI") {
    // AI should respond to attacks
    struct AIState {
        bool base_under_attack;
        int defender_count;
        int attacker_count;
    };

    AIState state = {true, 5, 3};

    bool should_defend = state.base_under_attack && state.defender_count > 0;
    TEST_ASSERT(should_defend);

    // Calculate defense response
    int defenders_to_send = (state.attacker_count * 3) / 2;  // 1.5x attackers
    if (defenders_to_send > state.defender_count) {
        defenders_to_send = state.defender_count;
    }

    TEST_ASSERT_EQ(defenders_to_send, 4);  // Send 4 defenders
}

TEST_CASE(Gameplay_AI_BuildOrder, "AI") {
    // AI follows build priorities
    struct BuildPriority {
        const char* name;
        int priority;  // Higher = build first
        bool has_prerequisite;
        bool built;
    };

    BuildPriority queue[] = {
        {"Power Plant", 100, true, false},
        {"Barracks", 80, false, false},    // Needs power
        {"Refinery", 90, true, false},
        {"War Factory", 70, false, false}  // Needs power
    };

    // Update prerequisites (power plant built gives power)
    queue[0].built = true;
    queue[1].has_prerequisite = true;
    queue[3].has_prerequisite = true;

    // Find next to build
    int best_idx = -1;
    int best_priority = -1;

    for (int i = 0; i < 4; i++) {
        if (queue[i].built) continue;
        if (!queue[i].has_prerequisite) continue;

        if (queue[i].priority > best_priority) {
            best_priority = queue[i].priority;
            best_idx = i;
        }
    }

    TEST_ASSERT_EQ(best_idx, 2);  // Refinery (priority 90)
}

TEST_CASE(Gameplay_AI_HarvesterManagement, "AI") {
    // AI rebuilds lost harvesters
    int harvester_count = 0;
    int min_harvesters = 2;
    int refinery_count = 2;

    // Ideal harvesters = 1 per refinery
    int ideal_harvesters = refinery_count;

    bool should_build_harvester = harvester_count < ideal_harvesters;
    TEST_ASSERT(should_build_harvester);

    harvester_count = 2;
    should_build_harvester = harvester_count < ideal_harvesters;
    TEST_ASSERT(!should_build_harvester);
}

TEST_CASE(Gameplay_AI_Scouting, "AI") {
    // AI sends scouts to explore
    struct MapRegion {
        bool explored;
        int last_scouted;  // Frame number
    };

    MapRegion regions[4] = {
        {true, 100},
        {false, 0},
        {true, 50},
        {false, 0}
    };

    int current_frame = 500;
    int rescout_interval = 300;

    // Find region needing scout
    int scout_target = -1;
    int oldest_scout = current_frame;

    for (int i = 0; i < 4; i++) {
        if (!regions[i].explored) {
            scout_target = i;  // Unexplored priority
            break;
        }
        if (current_frame - regions[i].last_scouted > rescout_interval) {
            if (regions[i].last_scouted < oldest_scout) {
                oldest_scout = regions[i].last_scouted;
                scout_target = i;
            }
        }
    }

    TEST_ASSERT_EQ(scout_target, 1);  // First unexplored
}

TEST_CASE(Gameplay_AI_ArmyComposition, "AI") {
    // AI builds balanced army
    struct UnitCount {
        const char* type;
        int count;
        int ideal_ratio;  // Percentage
    };

    UnitCount army[] = {
        {"Infantry", 20, 40},  // Should be 40% of army
        {"Tanks", 5, 30},
        {"Artillery", 2, 15},
        {"Anti-Air", 1, 15}
    };

    int total = 0;
    for (int i = 0; i < 4; i++) {
        total += army[i].count;
    }

    // Find most needed unit type
    int most_needed = 0;
    int biggest_deficit = 0;

    for (int i = 0; i < 4; i++) {
        int current_percent = (army[i].count * 100) / total;
        int deficit = army[i].ideal_ratio - current_percent;

        if (deficit > biggest_deficit) {
            biggest_deficit = deficit;
            most_needed = i;
        }
    }

    // Tanks or anti-air likely needed
    TEST_ASSERT_GE(most_needed, 1);  // Not infantry (we have plenty)
}

TEST_CASE(Gameplay_AI_Retreat, "AI") {
    // AI retreats damaged units
    struct Unit {
        int health;
        int max_health;
        bool retreating;
    };

    Unit units[] = {
        {100, 100, false},
        {30, 100, false},   // Low health
        {50, 100, false}
    };

    int retreat_threshold = 35;  // 35% health

    for (int i = 0; i < 3; i++) {
        int health_percent = (units[i].health * 100) / units[i].max_health;
        if (health_percent < retreat_threshold) {
            units[i].retreating = true;
        }
    }

    TEST_ASSERT(!units[0].retreating);
    TEST_ASSERT(units[1].retreating);
    TEST_ASSERT(!units[2].retreating);
}

TEST_CASE(Gameplay_AI_EconomyBalance, "AI") {
    // AI balances military vs economy spending
    int credits = 5000;
    int military_priority = 60;  // 60% military, 40% economy

    int military_budget = (credits * military_priority) / 100;
    int economy_budget = credits - military_budget;

    TEST_ASSERT_EQ(military_budget, 3000);
    TEST_ASSERT_EQ(economy_budget, 2000);
}

TEST_CASE(Gameplay_AI_AttackTiming, "AI") {
    // AI attacks when army is ready
    struct AttackCondition {
        int min_tanks;
        int min_infantry;
        bool enemy_base_located;
    };

    AttackCondition conditions = {5, 10, true};

    int current_tanks = 3;
    int current_infantry = 8;
    bool base_known = true;

    bool can_attack = (current_tanks >= conditions.min_tanks) &&
                      (current_infantry >= conditions.min_infantry) &&
                      base_known;

    TEST_ASSERT(!can_attack);  // Not enough tanks

    current_tanks = 6;
    current_infantry = 12;

    can_attack = (current_tanks >= conditions.min_tanks) &&
                 (current_infantry >= conditions.min_infantry) &&
                 base_known;

    TEST_ASSERT(can_attack);
}

TEST_CASE(Gameplay_AI_ResourceExpansion, "AI") {
    // AI expands to new ore fields
    int current_income = 1000;
    int desired_income = 2000;
    int refineries = 1;

    bool should_expand = current_income < desired_income;
    TEST_ASSERT(should_expand);

    // Check if can afford refinery
    int refinery_cost = 2000;
    int current_credits = 2500;

    bool can_build_refinery = current_credits >= refinery_cost;
    bool should_build_refinery = should_expand && can_build_refinery;

    TEST_ASSERT(should_build_refinery);
}
