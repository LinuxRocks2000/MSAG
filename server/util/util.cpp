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

uint8_t sixteen(char byte) {
    // gets the 4-bit representation of a single base 16 byte, so if you've got, like 0xFF, you'd want to
    // sixteen('F') * 16 + sixteen('F'). You can also use bitshifting if you feel like it, which is slightly more efficient.
    if (byte >= '0' && byte <= '9') {
        return byte - '0';
    }
    if (byte >= 'a' && byte <= 'f') {
        return 10 + byte - 'a';
    }
    if (byte >= 'A' && byte <= 'F') {
        return 10 + byte - 'A';
    }
    printf("BAD BASE-16 DECODE!");
    return 0;
}

std::string upperCase(std::string in) {
    for (size_t i = 0; i < in.size(); i ++) {
        if (in[i] >= 'a' && in[i] <= 'z') { // could be optimized, but is it really necessary? see that random stackoverflow
            // post where that guy says it's not necessary
            in[i] -= 'a';
            in[i] += 'A';
        }
    }
    return in;
}


std::string from16(std::string data) {
    std::string ret;
    ret.reserve(data.size() / 2); // allocate ONCE, I tell ye, ONCE
    for (size_t i = 0; i < data.length(); i += 2) {
        ret += (char)((sixteen(data[i]) << 4) + sixteen(data[i + 1]));
    }
    return ret;
}