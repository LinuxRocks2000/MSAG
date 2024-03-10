#pragma once

// player class. 
#include <string>
#include <playerdata.h>
#include <util/vector.hpp>
#include <memory>
#include <util/inputs.hpp>
#include <util/rect.hpp>


struct Space; // forward-dec


struct Player {
    uint64_t id;
    uint32_t spaceID;
    std::string name;
    bool active = false;

    PlayerStats stats;

    Inputs currentInputs;

    std::shared_ptr<Space> currentSpace;

    Rect shape {
        .x = 0.0,
        .y = 0.0,
        .width = 50.0, // This is a universal constant.
        .height = 50.0 // we may eventually want to use that fancy json2header.py to make this a proper definition that can be shared to the client JavaScript
        // and changed without much pain.
        // TODO: maybe change types.json and types.h to be a simple widespread constants.json/constants.h?
    };

    Rect oldShape { // TODO: set up a proper Player constructor
        .x = 0.0,
        .y = 0.0,
        .width = 50.0,
        .height = 50.0
    };
    // TODO: inventory

    int connectedSocket = -1; // MAKE SURE TO KEEP THIS AT -1 WHENEVER A CLIENT IS NOT CONNECTED TO THIS PLAYER!

    void update();
};