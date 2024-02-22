// definitions
// SERIOUS SECURITY TODO TODO TODO TODO: WE NEED TO ADD FORMAT ERROR HANDLING! Messages that don't fit in a format, right now, can trigger buffer overflows.
// BAD BAD BAD BAD BAD! We ABSOLUTEY need to ensure that size limitations ARE RESPECTED ON BUFFERS! This may mean changing from our current lowlevel char*s to a
// Cursor wrapper that just returns 0s when an illegal access would occur.
// THIS IS THE MOST IMPORTANT TODO IN THE PROGRAM RIGHT NOW! UNTIL THIS IS FIXED, IT TAKES ABSOLUTE PRECEDENCE!
// OTHER TODOS THAT CLAIM TO TAKE PRECEDENCE ARE LYING!
#include <util/protocol/protocol.hpp>
#include <util/websocketframe.hpp>


void protocol::ProtocolFrameBase::sendTo(SocketSendBuffer<>* sendBuffer) {
    char buf[getSize()];
    load(buf);
    WebSocketFrame f {std::string {buf, getSize()}}; // TODO: make this not do the stupid allocation
    f.sendTo(sendBuffer);
}