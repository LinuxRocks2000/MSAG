// definitions for the player class

#include <player.hpp>
#include <util/protocol/outgoing.hpp>
#include <space.hpp>


void Player::update() {
    if (currentInputs.up) {
        shape.y -= 2;
    }
    if (currentInputs.down) {
        shape.y += 2;
    }
    if (currentInputs.left) {
        shape.x -= 2;
    }
    if (currentInputs.right) {
        shape.x += 2;
    }
    if (shape != oldShape) {
        oldShape = shape;
        protocol::outgoing::Move message;
        message.x = shape.x;
        message.y = shape.y;
        message.id = spaceID;
        currentSpace -> sendToAll(message);
    }
}