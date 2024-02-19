#pragma once
#include <cstdint>
#include <string>

#define SPEED 30 // util.hpp can also be for project-wide definitions

uint64_t getMillis();

void msleep(long msec); // THANKS, STACKOVERFLOW (modified to get rid of the signals, I don't care about those)

uint8_t sixteen(char byte);

std::string upperCase(std::string in);

std::string from16(std::string data);

struct StringPair {
    std::string one;
    std::string two;
};
