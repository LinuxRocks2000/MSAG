// player class. 
#include <string>
#include <playerdata.h>


struct Player {
    uint64_t id;
    std::string name;

    PlayerStats stats;
    // TODO: inventory
};