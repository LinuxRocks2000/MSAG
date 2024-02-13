#include <util/buffersocketwriter.hpp>
#include <sys/socket.h> // for ::close
#include <unistd.h>
#include <stdio.h>
#include <cstring>


template<>
SocketSendBuffer<>::SocketSendBuffer(int s) {
    csocket = s;
}

template<size_t BufferSize>
void SocketSendBuffer<BufferSize>::write(const char* data, size_t length) {
    if (length + size < BufferSize) {
        for (size_t i = 0; i < length; i ++) {
            buffer[size + i] = data[i];
        }
        size += length;
    }
    else { // if the buffer is at 510, and we're trying to write "Hello World" (length 11)
        size_t writeAmount = BufferSize - size - 1; // writeAmount is now 1
        write(data, writeAmount); // write the first segment of the data ('H' in the example)
        flush(); // flush the buffer (size is now 0, all data has been sent to the client)
        write(data + writeAmount, length - writeAmount); // length - writeAmount is 10: we're writing "ello World".

        // this will recurse sanely for absurdly long data.
    }
}

template<>
void SocketSendBuffer<>::write(const char* data) {
    write(data, strlen(data));
}

template<>
void SocketSendBuffer<>::write(std::string data) {
    write(data.c_str(), data.size());
}

template<>
void SocketSendBuffer<>::write(char data) {
    write(&data, 1); // TODO: make this faster, right now it engages the full algorithm for a single-byte write.
}

template<>
void SocketSendBuffer<>::flush() { // always call this after you're done sending any frame of anything; it's never guaranteed what will be left in it.
    if (size != 0) { // if your data aligned perfectly with the buffer, the final flush call you MUST make will have nothing to flush. so just immediately return.
        send(csocket, buffer, size, 0);
        size = 0; // does NOT clean the buffer; make sure to pay attention to size at all times!
    }
}

template<>
void SocketSendBuffer<>::close() {
    shutdown(csocket, SHUT_RDWR);
    ::close(csocket); // just calling this will cause problems; always shutdown first.
}
