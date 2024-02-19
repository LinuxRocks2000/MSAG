// declarations for websocket framing

#pragma once
#include <util/util.hpp>
#include <util/buffersocketwriter.hpp>


struct WebSocketFrame {
    // BIG TODO: packet fragmentation! Right now we don't fragment at all!
    // ANOTHER BIG TODO: implement opcodes! Right now we totally ignore all of them, assuming binary for everything (and ignoring continuation). This
    // is a serious implementation flaw and MUST BE FIXED!

    // TODO: PING/PONG!
    bool fin = true;
    uint8_t rsv = 0; // 3-bit number representing the RSV bits
    bool mask = false;
    uint8_t opcode = 0x2; // 4-bit operation code
    uint64_t length = 0; // the payload should be unsigned and no larger than 64 bits; after all, a negative payload would hint at some serious endianness problems
    uint32_t maskData = 0; // keep this at 0 if mask is false.
    std::string payload; // MAKE SURE TO RESERVE, DANGIT! WE ARE *NOT* DOING 100 ALLOCATIONS TO RECEIVE A SINGLE SCUTTING FRAME!
    // TODO: verify that I used reserve properly.

    void sendTo(SocketSendBuffer<>* socket);

    void sendTo(int socket);

    WebSocketFrame(std::string frame);

    WebSocketFrame();
};