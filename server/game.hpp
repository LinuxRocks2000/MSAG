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

#include <pthread.h>
#include <atomic>
#include <memory>
#include "space.hpp"


struct Game {
    std::vector<std::shared_ptr<Space>> spaces; // the threads have different arrays, but the same data inside them.
    std::vector<std::shared_ptr<Room>> rooms;
    std::atomic<uint64_t> dataAge = 0;
    std::mutex dataEdit;
    uint64_t updateStagger = 0;

    static void* child(void* _me) {
        struct Game* self = (struct Game*)_me;
        std::vector<std::shared_ptr<Space>> localSpaces; // TODO: less inefficient
        uint64_t localAge = 0;
        while (true) {
            if (self -> dataAge > localAge) {
                self -> dataEdit.lock();
                localAge = (uint64_t)self -> dataAge;
                localSpaces = self -> spaces;
                self -> dataEdit.unlock();
            }
            uint64_t cTime = getMillis();
            // The process is pretty simple. Each child thread iterates over the spaces array, looking for unlocked spaces that need to update NOW. It locks and updates
            // them, then continues. As it goes, it's also constantly checking which doesn't need to update *now*, but will need to update soonest. It sleeps until
            // the soonest one will need execution, then does the whole thing over again.
            // TODO: check if this model is hotter than necessary.
            uint64_t soonest = cTime + 10000; // just something ridiculously unreasonable, ten seconds in the future
            for (size_t i = 0; i < localSpaces.size(); i ++) {
                if (cTime >= localSpaces[i] -> updateTime && localSpaces[i] -> mutex.try_lock()) {
                    localSpaces[i] -> update();
                    localSpaces[i] -> updateTime += 1000 / SPEED;
                    if (localSpaces[i] -> updateTime < soonest) {
                        soonest = localSpaces[i] -> updateTime;
                    }
                    localSpaces[i] -> mutex.unlock();
                }
                else if (localSpaces[i] -> updateTime < soonest) {
                    soonest = localSpaces[i] -> updateTime;
                }
            }
            msleep(soonest - cTime);
        }
    }

    void pushSpace(std::shared_ptr<Space> space) {
        space -> updateTime = (getMillis() / (1000/SPEED)) * (1000/SPEED); // clamp it
        space -> updateTime += updateStagger;
        updateStagger += 5; // so they all update slightly out-of-sync, meaning the threads stay warm but balance resource load efficiently
        dataEdit.lock();
        dataAge ++;
        spaces.push_back(space);
        dataEdit.unlock();
    }

    void spawnOff(int tCount) { // spawn threads
        pthread_t buffer;
        for (int i = 0; i < tCount; i ++) {
            pthread_create(&buffer, NULL, &child, (void*)this);
            pthread_detach(buffer);
        }
    }

    void block() { // run the worker thread on wherever this is called, blocking that thread permanently
        child((void*)this);
    }
};