CPP=g++

map/%.a: map/%.cpp
	$(CPP) -shared -Fpic -Wall -lpthread --std=c++20 -I. $< -o $@

util/build: 
	mkdir -p util/build

build:
	mkdir -p build

util/build/%.o: util/build util/%.cpp
	$(CPP) -c -Wall --std=c++20 -I. $< -o $@

server: build $(wildcard map/*.a) util/build/util.o util/build/protocol.o util/build/buffersocketwriter.o build/
	$(CPP) -c core.cpp -o build/core.o -I.
	$(CPP) -Wall $(wildcard util/build/*.o) $(wildcard build/*.o) -o server -lpthread --std=c++20 -I. -Wno-multichar