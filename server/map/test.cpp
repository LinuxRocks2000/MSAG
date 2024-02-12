#include <room.hpp>
#include <space.hpp>
#include <stdio.h>


extern "C" {
    void mapSetup(Room* room) {
        Space* s = new Space(1000, 1000);
        s -> addGround(Ground::makeBasicEarth(Rect {
            .x = 0,
            .y = 0,
            .width = 1000,
            .height = 1000,
        }));
        room -> addSpace(s);
        printf("hi\n");
    }
}