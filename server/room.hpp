/* By Tyler Clarke
    This work is protected by the GNU GPLv3 license, which is attached in the root directory of the project.
    TLDR; don't make closed-source stuff with my code, don't lie about writing it yourself, don't license it anything other than GPLv3.
    you can sell derivative works if you want (as long as the source code is provided), but that would be dumb.
    
    This is MSAG's room subsystem. Rooms are really just loaders and book-keeping agents - they manage players and maps.

    If you're lookin' to contribute, just ctrl+F for the word "TODO". There's a *lot* of TODO. Seriously. If your only contribution to the file is
    to add your name at the top, I'm not going to accept it. I'm looking at you, 1000D ;)
*/
#pragma once
#include <cstdint>
#include <space.hpp>
#include <memory>
#include <vector>
#include <player.hpp>

struct Game; // forward-dec

struct Room { // manages maps inside a game
    Game* game;
    void* handle;
    uint32_t spaceID = 1;
    bool allowAnonymous = true; // can anonymous players join this room?
    bool requireApproval = false; // do players have to be approved by superuser to join this room?
    std::vector<Player> players;
    std::vector<std::shared_ptr<Space>> mSpaces;

    Room(Game* g, const char* mapFile); // Takes a Game and the name of the map shared object it's gonna load

    virtual void addSpace(Space* s); // vtable lookups work across dynamic loads, so we can use member functions sanely! I love it when things just work.
};