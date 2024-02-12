#pragma once
// simple axis-aligned rectangles (there's no off-rotation in this game, at the moment. I may add it later.)
struct Rect {
    float x;
    float y;
    float width;
    float height;

    bool doesCollide(Rect& nebber) { // this is VERY low-overhead. 
        return x + width > nebber.x &&
            x < nebber.x + nebber.width &&
            y + height > nebber.y &&
            y < nebber.y + nebber.height;
    }
};