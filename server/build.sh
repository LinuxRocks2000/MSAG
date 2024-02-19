# simple barebones build script before I move to a proper makefile
# because the build command is gettin' bigger all the tiiiiime

cd util/protocol
python3 setup.py
cd ../../
g++ --shared -fPIC map/test.cpp -o map/test.a -I. -g
g++ -o server util/protocol/protocol.cpp util/protocol/outgoing.cpp util/*.cpp room.cpp game.cpp core.cpp -g -lpthread --std=c++20 -I.