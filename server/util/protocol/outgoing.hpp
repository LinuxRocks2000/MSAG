// AUTOGENERATED BY setup.py! DO NOT EDIT MANUALLY!
// By Tyler Clarke.
// This software is protected by GPLv3. That means you have to provide the source for anything that uses my code, and you have to give me credit.
// You can make money off this if you want, but that would be dumb.

// This is the declaration file for the outgoing protocol.

#pragma once
#include <util/protocol/protocol.hpp>
namespace protocol::outgoing {
struct TestFrame : ProtocolFrameBase {
static uint8_t opcode;
uint32_t number;
uint32_t numbertwo;
int32_t signedint;
std::string letterz;
float32_t floating;
TestFrame(const char* data);
TestFrame();
void load(char* buffer);
size_t getSize();
};
struct Welcome : ProtocolFrameBase {
static uint8_t opcode;
Welcome(const char* data);
Welcome();
void load(char* buffer);
size_t getSize();
};
struct RoomCreated : ProtocolFrameBase {
static uint8_t opcode;
uint32_t creator;
uint32_t roomid;
RoomCreated(const char* data);
RoomCreated();
void load(char* buffer);
size_t getSize();
};
struct SpaceSet : ProtocolFrameBase {
static uint8_t opcode;
uint32_t spaceID;
float32_t spaceWidth;
float32_t spaceHeight;
SpaceSet(const char* data);
SpaceSet();
void load(char* buffer);
size_t getSize();
};
struct IdSet : ProtocolFrameBase {
static uint8_t opcode;
uint32_t objectID;
IdSet(const char* data);
IdSet();
void load(char* buffer);
size_t getSize();
};
struct GroundSet : ProtocolFrameBase {
static uint8_t opcode;
float32_t x;
float32_t y;
float32_t width;
float32_t height;
uint32_t id;
uint32_t type;
GroundSet(const char* data);
GroundSet();
void load(char* buffer);
size_t getSize();
};
struct PlayerSet : ProtocolFrameBase {
static uint8_t opcode;
float32_t x;
float32_t y;
uint32_t id;
float32_t health;
float32_t maxHealth;
PlayerSet(const char* data);
PlayerSet();
void load(char* buffer);
size_t getSize();
};
struct Move : ProtocolFrameBase {
static uint8_t opcode;
float32_t x;
float32_t y;
uint32_t id;
Move(const char* data);
Move();
void load(char* buffer);
size_t getSize();
};
}
