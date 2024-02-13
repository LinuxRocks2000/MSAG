#pragma once

#include <cstddef>
#include <string>


template <size_t BufferSize=512> // TODO: pick a better buffer size. Got to do performance tests to determine which one makes most sense for our uses.
struct SocketSendBuffer { // buffers sends on a socket, Nagle-style, to minimize syscalls.
    char buffer[BufferSize]; // very small buffer for now; increase size later if necessary (most messages will be under a kilobyte)
    int size = 0;
    int csocket;

    SocketSendBuffer(int s);

    void write(const char* data, size_t length);

    void write(const char* data);

    void write(std::string data);

    void write(char data);

    void flush();

    void close();
};
