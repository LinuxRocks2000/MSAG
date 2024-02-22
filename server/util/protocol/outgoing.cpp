// AUTOGENERATED BY setup.py! DO NOT EDIT MANUALLY!
// By Tyler Clarke.
// This software is protected by GPLv3. That means you have to provide the source for anything that uses my code, and you have to give me credit.
// You can make money off this if you want, but that would be dumb.

// This is the definition file for the outgoing protocol.

#include <util/protocol/outgoing.hpp>
protocol::outgoing::TestFrame::TestFrame() {}
protocol::outgoing::TestFrame::TestFrame(const char* data) {
size_t len;
for (size_t i = 0; i < sizeof(uint32_t); i ++) {
((char*)&number)[i] = data[i];
}
data += sizeof(uint32_t);
for (size_t i = 0; i < sizeof(uint32_t); i ++) {
((char*)&numbertwo)[i] = data[i];
}
data += sizeof(uint32_t);
for (size_t i = 0; i < sizeof(int32_t); i ++) {
((char*)&signedint)[i] = data[i];
}
data += sizeof(int32_t);
len = data[0]; data++;
if (len == 255) {len = ((uint32_t*)&data)[0]; data += 4;}
letterz.reserve(len);
for (size_t i = 0; i < len; i ++) {letterz += data[i];}
data += len;for (size_t i = 0; i < sizeof(float32_t); i ++) {
((char*)&floating)[i] = data[i];
}
data += sizeof(float32_t);
}
void protocol::outgoing::TestFrame::load(char* buffer) {
buffer[0] = 0;
buffer ++; // clever C hack: rather than worrying about current index, we can just consume a byte of the buffer.
// This is very fast and makes life a lot easier.
for (uint8_t i = 0; i < sizeof(uint32_t); i ++){buffer[i] = ((char*)&number)[i];}
buffer += sizeof(uint32_t); // see above
for (uint8_t i = 0; i < sizeof(uint32_t); i ++){buffer[i] = ((char*)&numbertwo)[i];}
buffer += sizeof(uint32_t); // see above
for (uint8_t i = 0; i < sizeof(int32_t); i ++){buffer[i] = ((char*)&signedint)[i];}
buffer += sizeof(int32_t); // see above
size_t size = letterz.size();
if (size < 255) {
    buffer[0] = size; buffer++;
}
else {
    buffer[0] = 255;
    buffer ++;
    ((uint32_t*)&buffer)[0] = size;
    buffer += 4;
}
for (size_t i = 0; i < size; i ++) {buffer[i] = letterz[i];}
buffer += size;

for (uint8_t i = 0; i < sizeof(float32_t); i ++){buffer[i] = ((char*)&floating)[i];}
buffer += sizeof(float32_t); // see above
}

size_t protocol::outgoing::TestFrame::getSize() {
return 1 + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(int32_t) + (letterz.size() + (letterz.size() < 255 ? 1 : 5)) + sizeof(float32_t);
}
protocol::outgoing::TestFrame2::TestFrame2() {}
protocol::outgoing::TestFrame2::TestFrame2(const char* data) {
size_t len;
len = data[0]; data++;
if (len == 255) {len = ((uint32_t*)&data)[0]; data += 4;}
text.reserve(len);
for (size_t i = 0; i < len; i ++) {text += data[i];}
data += len;for (size_t i = 0; i < sizeof(float32_t); i ++) {
((char*)&number)[i] = data[i];
}
data += sizeof(float32_t);
}
void protocol::outgoing::TestFrame2::load(char* buffer) {
buffer[0] = 1;
buffer ++; // clever C hack: rather than worrying about current index, we can just consume a byte of the buffer.
// This is very fast and makes life a lot easier.
size_t size = text.size();
if (size < 255) {
    buffer[0] = size; buffer++;
}
else {
    buffer[0] = 255;
    buffer ++;
    ((uint32_t*)&buffer)[0] = size;
    buffer += 4;
}
for (size_t i = 0; i < size; i ++) {buffer[i] = text[i];}
buffer += size;

for (uint8_t i = 0; i < sizeof(float32_t); i ++){buffer[i] = ((char*)&number)[i];}
buffer += sizeof(float32_t); // see above
}

size_t protocol::outgoing::TestFrame2::getSize() {
return 1 + (text.size() + (text.size() < 255 ? 1 : 5)) + sizeof(float32_t);
}
