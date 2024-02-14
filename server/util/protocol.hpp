// Protocol manager.
#include <util/buffersocketwriter.hpp>
#include <util/util.hpp>


namespace protocol {
    template <typename T>
    struct FatPointer { // simple fat pointer type
        size_t length;
        T* data;
    };


    struct Element {
        uint16_t type = 'op'; // also known as two chars. fancy C hack; allows blazing fast comparisons and is easy to work with.
        union {
            FatPointer<char> string; // if type == 'ss'
            uint64_t unsignedInt; // if type == 'u8' or 'u4' (if u4, the 4 most significant bytes are ignored; it's parsed as a 32-bit number)
            int64_t signedInt;
            double floating; // if type == 'f8' or 'f4' (see above for behavior with f4)
            uint8_t operation; // if type == 'op'
        };

        size_t getSize();

        void render(char* buffer);
    };


    typedef FatPointer<Element> Frame;


    Frame makeFrame(uint8_t operation, const char* format, ...);

    
    size_t frameLength(Frame* frame);


    void renderFrame(Frame* frame, char* buffer); // buffer should be stack allocated or something; large enough to hold the frame (maybe stack allocate if it's small, and heap
    // allocate if it's large?)
};