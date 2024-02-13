# simple barebones build script before I move to a proper makefile
# because the build command is gettin' bigger all the tiiiiime
rm -rf build/
mkdir build/
g++ --shared -fPIC map/test.cpp -o map/test.a -I. -g
g++ -o server util/buffersocketwriter.cpp util/protocol.cpp util/util.cpp room.cpp game.cpp core.cpp -g -lpthread --std=c++20 -I.