#include "util/net.hpp"


struct Game { // container class

};


struct Handler {
    void onWebsocketUp(uint64_t id) {

    }
};


int main() {
    NetworkServer<Handler, Game> ns(3000);
    Game game;
    ns.spawnOff(16, &game);
    ns.block();
}