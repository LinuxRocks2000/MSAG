/* By Tyler Clarke
    This work is protected by the GNU GPLv3 license, which is attached in the root directory of the project.
    TLDR; don't make closed-source stuff with my code, don't lie about writing it yourself, don't license it anything other than GPLv3.
    you can sell derivative works if you want (as long as the source code is provided), but that would be dumb.
    
    This is the actual code that manages the game itself. It uses the same thread-pooling trick as net.hpp.
    Update times of each Space should be staggered by 5ms or so. This allows the threads to stay a *little* hot the entire time, as opposed to *really* hot once every
    33ms. That'll significantly improve performance.

    If you're lookin' to contribute, just ctrl+F for the word "TODO". There's a *lot* of TODO. Seriously. If your only contribution to the file is
    to add your name at the top, I'm not going to accept it. I'm looking at you, 1000D ;)
*/

#pragma once
#include <pthread.h>
#include <atomic>
#include <vector>
#include <memory>
#include "space.hpp"
#include "room.hpp"


struct Game {
    std::vector<std::shared_ptr<Space>> spaces; // the threads have different arrays, but the same data inside them.
    std::vector<std::shared_ptr<Room>> rooms;
    std::atomic<uint64_t> dataAge = 0;
    std::mutex dataEdit;
    uint64_t updateStagger = 0;

    static void* child(void* _me); // Worker thread

    void pushSpace(std::shared_ptr<Space> space); // Push a new space into this Room

    void spawnOff(int tCount); // spawn threads

    void block(); // run the worker thread on wherever this is called, blocking that thread permanently
};