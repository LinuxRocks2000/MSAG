#include <space.hpp>


uint32_t Space::allocateID() {
    topSpaceID ++;
    return topSpaceID - 1;
}

Space::Space(float w, float h) {
    spaceID = 0;
    width = w;
    height = h;
}

void Space::addGround(Ground g) { // TODO: Mutexing.
    g.spaceID = allocateID();
    groundLayer.push_back(g);
    protocol::outgoing::GroundSet message;
    message.x = g.shape.x; // TODO: (this is probably written elsewhere but it's relevant here) Add abstract types (rect, vector, etc) to the protocol.
    message.y = g.shape.y;
    message.width = g.shape.width;
    message.height = g.shape.height;
    message.type = g.type;
    message.id = g.spaceID;
    sendToAll(message);
}

void Space::addPlayer(Player p) {
    p.spaceID = allocateID();
    protocol::outgoing::PlayerSet message;
    message.x = p.shape.x;
    message.y = p.shape.y;
    message.id = p.spaceID;
    message.health = p.stats.health;
    message.maxHealth = p.stats.maxHealth;
    sendToAll(message);
    players.push_back(p);
}

void Space::update() {
    for (size_t i = 0; i < players.size(); i ++) {
        players[i].update();
    }
}

Ground* Space::makeBasicEarth(Rect r) {
    Ground g = {
        .shape = r,
        .moveHindrance = 0.2, // a little higher than Rock.
        .gardenPotential = 0.3, // it's pretty good for gardening too
        .type = TYPE_GROUND_SOIL
    };
    addGround(g);
    return &groundLayer[groundLayer.size() - 1]; // WARNING: THIS POINTER IS NOT TRUSTWORTHY!
    // IT *WILL* BE INVALIDATED NEXT TIME WE CALL ANY GROUND-ADDING FUNCTION! DON'T BE STUPID!
}