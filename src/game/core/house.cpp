/**
 * House/Faction Implementation
 *
 * Provides faction-related utility functions.
 */

#include "game/house.h"
#include <cstring>

// =============================================================================
// House Information Table
// =============================================================================

const HouseInfo HouseInfoTable[HOUSE_COUNT] = {
    // Name,      Full Name,     Side,         Color,        Suffix
    {"SPAIN",     "Spain",       SIDE_ALLIED,  PCOLOR_GOLD,  "sp"},
    {"GREECE",    "Greece",      SIDE_ALLIED,  PCOLOR_LTBLUE,"gr"},
    {"USSR",      "USSR",        SIDE_SOVIET,  PCOLOR_RED,   "su"},
    {"ENGLAND",   "England",     SIDE_ALLIED,  PCOLOR_GREEN, "en"},
    {"UKRAINE",   "Ukraine",     SIDE_SOVIET,  PCOLOR_ORANGE,"uk"},
    {"GERMANY",   "Germany",     SIDE_ALLIED,  PCOLOR_GREY,  "ge"},
    {"FRANCE",    "France",      SIDE_ALLIED,  PCOLOR_BLUE,  "fr"},
    {"TURKEY",    "Turkey",      SIDE_ALLIED,  PCOLOR_BROWN, "tu"},
    {"GOODGUY",   "GoodGuy",     SIDE_ALLIED,  PCOLOR_BLUE,  ""},
    {"BADGUY",    "BadGuy",      SIDE_SOVIET,  PCOLOR_RED,   ""},
    {"NEUTRAL",   "Neutral",     SIDE_NEUTRAL, PCOLOR_GOLD,  ""},
    {"SPECIAL",   "Special",     SIDE_NEUTRAL, PCOLOR_GOLD,  ""},
    {"MULTI1",    "Multi1",      SIDE_ALLIED,  PCOLOR_GOLD,  "m1"},
    {"MULTI2",    "Multi2",      SIDE_ALLIED,  PCOLOR_LTBLUE,"m2"},
    {"MULTI3",    "Multi3",      SIDE_ALLIED,  PCOLOR_RED,   "m3"},
    {"MULTI4",    "Multi4",      SIDE_ALLIED,  PCOLOR_GREEN, "m4"},
    {"MULTI5",    "Multi5",      SIDE_ALLIED,  PCOLOR_ORANGE,"m5"},
    {"MULTI6",    "Multi6",      SIDE_ALLIED,  PCOLOR_GREY,  "m6"},
    {"MULTI7",    "Multi7",      SIDE_ALLIED,  PCOLOR_BLUE,  "m7"},
    {"MULTI8",    "Multi8",      SIDE_ALLIED,  PCOLOR_BROWN, "m8"},
};

// =============================================================================
// Utility Functions
// =============================================================================

extern "C" {

SideType House_Side(HousesType house) {
    if (house < 0 || house >= HOUSE_COUNT) {
        return SIDE_NONE;
    }
    return HouseInfoTable[house].Side;
}

PlayerColorType House_Default_Color(HousesType house) {
    if (house < 0 || house >= HOUSE_COUNT) {
        return PCOLOR_NONE;
    }
    return HouseInfoTable[house].Color;
}

const char* House_Name(HousesType house) {
    if (house < 0 || house >= HOUSE_COUNT) {
        return "NONE";
    }
    return HouseInfoTable[house].Name;
}

const char* Side_Name(SideType side) {
    switch (side) {
        case SIDE_ALLIED: return "Allied";
        case SIDE_SOVIET: return "Soviet";
        case SIDE_NEUTRAL: return "Neutral";
        default: return "None";
    }
}

int Houses_Allied(HousesType house1, HousesType house2) {
    // Same house is always allied
    if (house1 == house2) return 1;

    // Neutral is neutral to everyone
    if (house1 == HOUSE_NEUTRAL || house2 == HOUSE_NEUTRAL) return 1;

    // Same side houses are allied
    SideType side1 = House_Side(house1);
    SideType side2 = House_Side(house2);
    return (side1 == side2) ? 1 : 0;
}

int Houses_Enemy(HousesType house1, HousesType house2) {
    return !Houses_Allied(house1, house2);
}

HousesType House_From_Name(const char* name) {
    if (name == nullptr) return HOUSE_NONE;

    for (int i = 0; i < HOUSE_COUNT; i++) {
        if (strcmp(HouseInfoTable[i].Name, name) == 0) {
            return (HousesType)i;
        }
    }
    return HOUSE_NONE;
}

int House_Is_Multi(HousesType house) {
    return (house >= HOUSE_MULTI1 && house <= HOUSE_MULTI8) ? 1 : 0;
}

} // extern "C"
