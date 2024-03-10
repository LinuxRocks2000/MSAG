#include <room.hpp>
#include <space.hpp>
#include <stdio.h>
#include <memory>


extern "C" {
    void mapSetup(Room* room) {
        std::shared_ptr<Space> s = room -> addSpace(1000, 1000);
        s -> makeBasicEarth(Rect {
            .x = 0,
            .y = 0,
            .width = 1000,
            .height = 1000,
        });
        printf("Map file loaded\n");
    }
}