# simple barebones build script before I move to a proper makefile
# because the build command is gettin' bigger all the tiiiiime

cd util/protocol
python3 setup.py
cd ../../
python3 json2header.py types.json types.h -p TYPE # generate the ground types header for Space.hpp
cp types.json ../client/
g++ --shared -fPIC map/test.cpp -o map/test.a -I. -g
g++ -o server util/protocol/*.cpp util/*.cpp space.cpp player.cpp room.cpp game.cpp core.cpp -g -lpthread --std=c++20 -I.

cp util/protocol/manifest.json ../client/