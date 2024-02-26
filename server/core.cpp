/* By Tyler Clarke
    This work is protected by the GNU GPLv3 license, which is attached in the root directory of the project.
    TLDR; don't make closed-source stuff with my code, don't lie about writing it yourself, don't license it anything other than GPLv3.
    you can sell derivative works if you want (as long as the source code is provided), but that would be dumb.
    
    This is the main server file for my third "big" video game - Multiplayer Survival Adventure Game, the sequel to MMOSG in exactly no respects.
    I opted for C++ instead of Rust because Rust has some performance concerns for multiplayer games that, to get around, I would have to write exactly as much
    boilerplate in a less forgiving language. Also I like the C++ threading api and the C++ file api and the C++ socket api,
    and I hate those apis in rust (and don't want to write the boilerplate to make it better, because I may as well just write it in C++ in that case)

    If you're lookin' to contribute, just ctrl+F for the word "TODO". There's a *lot* of TODO. Seriously. If your only contribution to the file is
    to add your name at the top, I'm not going to accept it. I'm looking at you, 1000D ;)
*/

// BIG TODO: Finish file separation (no definitions in hpp files) and thoroughly comment all the hpp files. Right now it's pretty sparse and you have to read
// both the source file and the header file to make any sense of it; this must change!

// also TODO: do all the license comments at the tops of files. really this project needs a *lot* of commenting.

// WARNING: This application's network code is generally endianness-specific! The client will work anywhere numbers are sold, but the server is a bit pickier.
// If the server isn't working, the FIRST THING YOU SHOULD DO is test endianness!

#include <time.h>
#include <cstdint>
#include "util/net.hpp"
#include "game.hpp"
#include "room.hpp"
#include "space.hpp"
#include "util/util.hpp"
#include <util/protocol/outgoing.hpp>
#include <util/protocol/incoming.hpp>
#include <player.hpp>


void loadSpaceTo(std::shared_ptr<Space> space, int socket) { // TODO: make this a member function of Space (after making space.hpp and space.cpp separate)
    protocol::outgoing::SpaceSet s;
    s.spaceID = space -> spaceID;
    s.spaceWidth = space -> width;
    s.spaceHeight = space -> height;
    s.sendTo(socket);
}


struct Handler {
    Game* game;
    Player* cPlayer;

    Handler(Game* argument) : game{argument} {

    }

    int socket;

    void onWebsocketUp(uint64_t id, int m_socket) {
        socket = m_socket;
        printf("Got websocket up\n");
    }

    void httpRequest(HTTPRequest request, HTTPResponse* response) {
        if (request.uri == "/") {
            response -> code = 200;
            response -> data = "<!DOCTYPE html><title>Information</title><p>You have reached an MSAG server. This is just a server; you'll"
            " need to get your client somewhere else and specify this server when you connect.</p>";
            response -> contentType = HTTP_CONTENT_TYPE_HTML;
        } // TODO: Finish implementing the API found in README. It's a pretty sane API.
        else {
            response -> code = 404;
            response -> data = "File Not Found";
            response -> contentType = HTTP_CONTENT_TYPE_PLAINTEXT;
        }
    }

    bool mayWebsocketUpgrade(HTTPRequest* request) { // if it returns true, the upgrade will continue OR this object will be destroyed, so it's okay to start making
        // decisions right away.
        if (request -> uri == "/game") { // later, add an int to discriminate what endpoint we on.
            return true;
        }
        return false; // just fer testin' up
    }

    void gotWebsocketMessage(WebSocketFrame frame) {
        uint8_t opcode = frame.payload[0];
        const char* databuf = frame.payload.c_str() + 1;
        if (opcode == protocol::incoming::Init::opcode) { // no reason to unload the data buffer, Init has no data
            printf("Client init. Let's welcome them!");
            protocol::outgoing::Welcome welcome;
            welcome.sendTo(socket);
        }
        else if (opcode == protocol::incoming::RoomCreate::opcode) {
            protocol::incoming::RoomCreate message(databuf);
            auto room = std::shared_ptr<Room>(new Room(game, ("map/" + message.mapName).c_str(), message.roomName)); // TODO: error handling. seriously.
            // TODO: make it so the user doesn't manually specify a map file, just puts in a map name and the program sanitizes it and converts it to map filename
            // (and checks if it does, in fact, exist)
            room -> creator = message.creator; // TODO: collision checks (to avoid creator-check crossovers)
            game -> pushRoom(room);
            protocol::outgoing::RoomCreated output;
            output.roomid = room -> id;
            output.creator = message.creator; // TODO: collision checks (to avoid creator-check crossovers)
            output.sendTo(socket);
        }
        else if (opcode == protocol::incoming::RoomJoin::opcode) {
            protocol::incoming::RoomJoin message(databuf);
            Player p; // create a new player instance
            p.id = message.playerID;
            printf("Created new %u\n", p.id);
            p.name = message.playerName;
            for (size_t i = 0; i < game -> rooms.size(); i ++) {
                if (game -> rooms[i] -> id == message.roomid) { // we've found the room they wanna join, let's do the thing!
                    p.shape.x = game -> rooms[i] -> mSpaces[0] -> defaultX;
                    p.shape.y = game -> rooms[i] -> mSpaces[0] -> defaultY;
                    p.currentSpace = game -> rooms[i] -> mSpaces[0];
                    game -> rooms[i] -> mSpaces[0] -> addPlayer(p); // the mSpaces dimension contains a referential map to the spaces array in Game
                    // this is an overly fancy way to say "wow, we can search in two ways rather than just one!"
                }
            }
        }
        else if (opcode == protocol::incoming::RoomConnect::opcode) {
            protocol::incoming::RoomConnect message(databuf);
            cPlayer = game -> playerSearch(message.playerID);
            if (cPlayer != nullptr) {
                cPlayer -> active = true;
                cPlayer -> currentSpace -> playerCount ++;
                loadSpaceTo(cPlayer -> currentSpace, socket);
                protocol::outgoing::IdSet m; // link up the player with the player object in the space
                m.objectID = cPlayer -> spaceID;
                m.sendTo(socket);
                printf("Player connect %u\n", message.playerID);
            }
            else {
                printf("nonexistent player connect %u?\n", message.playerID);
            }
        }
    }
};


int main() {
    NetworkServer<Handler, Game> ns(3001);
    Game game;
    ns.spawnOff(8, &game);
    game.spawnOff(7);
    game.block();
}