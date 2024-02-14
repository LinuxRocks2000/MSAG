// Protocol manager.
#include <util/buffersocketwriter.hpp>
#include <stdio.h>
#include <string.h>
#include <util/util.hpp>
#include <stdarg.h>
#include <memory>
#include <util/protocol.hpp>


namespace protocol {
    size_t Element::getSize() {
        switch (type) {
            case 'op': return 1;
            case 'ss': return string.length + (string.length < 255 ? 1 : 4);
            case 'u8': return 8;
            case 'u4': return 4;
            case 'f8': return 8;
            case 'f4': return 4;
        };
        return 0; // it's unvalid
    }

    void Element::render(char* buffer) {
        if (type == 'op') {
            buffer[0] = operation;
        }
        else if (type == 'ss') {
            if (string.length < 255) {
                buffer[0] = (uint8_t)string.length;
                buffer ++;
            }
            else {
                ((uint32_t*)buffer)[0] = string.length;
                buffer += 4;
            }
            for (size_t i = 0; i < string.length; i ++) {
                buffer[i] = string.data[i];
            }
        }
        else if (type == 'u4') {
            ((uint32_t*)buffer)[0] = unsignedInt;
        }
        else if (type == 'u8') {
            ((uint64_t*)buffer)[0] = unsignedInt;
        }
        else if (type == 'i4') {
            ((int32_t*)buffer)[0] = signedInt;
        }
        else if (type == 'i8') {
            ((int64_t*)buffer)[0] = signedInt;
        }
        else if (type == 'f4') {
            ((float*)buffer)[0] = floating;
        }
        else if (type == 'f8') {
            ((double*)buffer)[0] = floating;
        }
        else {
            printf("BAD TYPE %c%c\n", operation >> 8, operation & 255);
        }
    }


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
        for (size_t i = 2; i < frameLen; i += 2) {
            ret.data[i].type = (uint16_t)format[i] << 8 + format[i + 1];
            if (format[i] == 'u') {
                ret.data[i].unsignedInt = va_arg(args, uint64_t); // ditty-o
            }
            else if (format[i] == 'f') {
                ret.data[i].floating = va_arg(args, double); // works for any float or double
            }
            else if (format[i] == 'i') {
                ret.data[i].signedInt = va_arg(args, int64_t); // ditty-boo
            }
            else if (format[i] == 's') {
                if (format[i + 1] == 's') { // premade fat-string
                    ret.data[i].string = va_arg(args, FatPointer<char>);
                }
                else if (format[i + 1] == 'c') {
                    const char* cstring = va_arg(args, const char*);
                    ret.data[i].string = FatPointer<char> {
                        .length = strlen(cstring),
                        .data = strdup(cstring)
                    };
                }
            }
            else {
                printf("BAD FORMAT\n");
            }
        }
        va_end(args);
        return ret;
    }

    size_t frameLength(Frame* frame) { // operation code is the first element in a valid Frame, so we need no extra opcode logic
        size_t ret = 0;
        for (size_t i = 0; i < frame -> length; i ++) {
            ret += frame -> data[i].getSize();
        }
        return ret;
    }

    void renderFrame(Frame* frame, char* buffer) {
        size_t soFar = 0;
        for (size_t i = 0; i < frame -> length; i ++) {
            frame -> data[i].render(buffer + soFar);
            soFar += frame -> data[i].getSize();
        }
    }
};