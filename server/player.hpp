#pragma once

// player class. 
#include <string>
#include <playerdata.h>
#include <util/vector.hpp>
#include <memory>


struct Space; // forward-dec


struct Player {
    uint64_t id;
    uint32_t spaceID;
    std::string name;
    bool active = false;

    PlayerStats stats;

    std::shared_ptr<Space> currentSpace;

    Rect shape {
        .x = 0.0,
        .y = 0.0,
        .width = 50.0, // TO BE CHANGED
        .height = 50.0
    };
    // TODO: inventory
};