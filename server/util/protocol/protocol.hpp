#pragma once
#include <util/buffersocketwriter.hpp>
#include <stdint.h>
typedef float float32_t;
typedef double float64_t;
#define uint64_t_TCHAR 'U'
#define int64_t_TCHAR 'I'

// Core code for protocol frames.
namespace protocol {
    struct ProtocolFrameBase {
        // the =0 means this is a *pure virtual* function - a virtual function that is *only* defined in subclasses. Possessing those makes this officially
        // an abstract class.
        virtual void load(char* buffer) = 0; // defined by subclasses

        virtual size_t getSize() = 0; // defined by subclasses
        
        void sendTo(SocketSendBuffer<>* buffer); // builds a websocket frame, loads into it, and sends it;
        // a very nice self-contained function with simple memory guarantees. Defined in protocol.cpp.

        virtual ~ProtocolFrameBase() = default; // if the destructor isn't virtual, destructors in subclasses won't be called. obviously a problem.
    };
}