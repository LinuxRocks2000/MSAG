// definitions
#include <util/protocol/protocol.hpp>
#include <util/websocketframe.hpp>


void protocol::ProtocolFrameBase::sendTo(SocketSendBuffer<>* sendBuffer) {
    char buf[getSize()];
    load(buf);
    WebSocketFrame f {std::string {buf, getSize()}}; // TODO: make this not do the stupid allocation
    f.sendTo(sendBuffer);
}