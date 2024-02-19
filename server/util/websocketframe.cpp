// websocket framing definitions
// this is part of my gradual efforts to make net.hpp a multifile blob with sane linking (and thus optimizable w/makefile)
// rather than one behemoth

#include <util/websocketframe.hpp>


void WebSocketFrame::sendTo(SocketSendBuffer<>* socket) {
    // TODO: check if it's worthwhile to use a raw socket and a stack-allocated buffer (only really valid for )
    socket -> write((fin << 7) // move the FIN bit to the MSB
                | (rsv << 4) // shift RSV up to fill the second, third, and fourth MSBs
                | (opcode)); // shove the opcode in
    // The server won't ever mask, because that's pointless. If we ever need masking for some unfathomable reason, add it. for now, though - no.
    if (length <= 125) {
        socket -> write(length);
    }
    else if (length < 65536) { // if it'll fit in the 16-bit int
        socket -> write(126);
        socket -> write((char*)&length, 2);
    }
    else {
        socket -> write(127);
        socket -> write((char*)&length, 8);
    }
    socket -> write(payload);

    socket -> flush(); // always gotta call this
}

void WebSocketFrame::sendTo(int socket) {
    SocketSendBuffer buffer(socket);
    sendTo(&buffer);
}

WebSocketFrame::WebSocketFrame(std::string frame) {
    length = frame.size();
    payload = frame;
}

WebSocketFrame::WebSocketFrame(){}