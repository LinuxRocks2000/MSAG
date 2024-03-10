/* By Tyler Clarke
    This work is protected by the GNU GPLv3 license, which is attached in the root directory of the project.
    TLDR; don't make closed-source stuff with my code, don't lie about writing it yourself, don't license it anything other than GPLv3.
    you can sell derivative works if you want (as long as the source code is provided), but that would be dumb.
    
    This is MSAG's room subsystem. Rooms are really just loaders and book-keeping agents - they manage players and maps.

    If you're lookin' to contribute, just ctrl+F for the word "TODO". There's a *lot* of TODO. Seriously. If your only contribution to the file is
    to add your name at the top, I'm not going to accept it. I'm looking at you, 1000D ;)
*/
#include <dlfcn.h>
#include <memory>
#include <room.hpp>
#include <game.hpp>


Room::Room(Game* g, const char* mapFile, std::string n) { // it requires a game and a shared object file. TODO: add an implementation where a Room can load from a Game and
    // a roomsave file (maybe with a defaulted bool in this function?)
    game = g;
    handle = dlopen(mapFile, RTLD_LAZY | RTLD_NOW); // TODO: check the dlopen flags in detail; we might be missing something useful!
    void(*mapFn)(Room*) = (void(*)(Room*))dlsym(handle, "mapSetup");
    if (mapFn == NULL) {
        printf("FAILED TO LOAD MAP.\n");
        dlclose(handle);
        return;
    }
    mapFn(this);
    name = n;
}

extern "C" {
    void Room::addSpace(Space* s) {
        auto ptr = std::shared_ptr<Space>(s);
        game -> pushSpace(ptr);
        mSpaces.push_back(ptr);
    }

    std::shared_ptr<Space> Room::addSpace(float width, float height) { // creates the space for you. Isn't it nice?
        auto s = std::shared_ptr<Space>(new Space(width, height));
        game -> pushSpace(s);
        mSpaces.push_back(s);
        return s;
    }
}