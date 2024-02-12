#pragma once
#include <cstdint>

#define SPEED 30 // util.hpp can also be for project-wide definitions

uint64_t getMillis();

void msleep(long msec); // THANKS, STACKOVERFLOW (modified to get rid of the signals, I don't care about those)