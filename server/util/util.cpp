#include "util.hpp"
#include <time.h>
#include <cstdint>

uint64_t getMillis() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

void msleep(long msec) // THANKS, STACKOVERFLOW (modified to get rid of the signals, I don't care about those)
{
    struct timespec ts;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    nanosleep(&ts, NULL);
}