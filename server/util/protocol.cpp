// Protocol manager.
#include <util/buffersocketwriter.hpp>
#include <stdio.h>
#include <string.h>
#include <util/util.hpp>
#include <stdarg.h>
#include <memory>


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

        size_t getSize() {
            switch (type) {
                case 'op': return 1;
                case 'ss': return string.length + (string.length < 255 ? 1 : 4);
                case 'u8': return 8;
                case 'u4': return 4;
                case 'f8': return 8;
                case 'f4': return 4;
            };
        }

        void render(char* buffer) {

        }
    };


    typedef FatPointer<Element> Frame;


    Frame makeFrame(uint8_t operation, const char* format, ...) {
        va_list args;
        va_start(args, format);
        size_t formatLen = strlen(format);
        size_t frameLen = formatLen/2 + 1;
        Frame ret {
            .length = frameLen, // +1 for the opcode
            .data = (Element*)malloc(sizeof(Element) * frameLen)
        };
        ret.data[0] = Element {
            .type = 'op',
            .operation = operation
        };
        for (size_t i = 1; i < frameLen; i ++) {
            ret.data[i].type = (uint16_t)format[i] << 8 + format[i + 1];
            if (format[i] == 'u') {
                ret.data[i].unsignedInt = va_arg(args, uint64_t); // ditty-o
            }
            else if (format[i] == 'f') {
                ret.data[i].floating = va_arg(args, double); // works for any float or double
            }
        }
        va_end(args);
        return ret;
    }
};