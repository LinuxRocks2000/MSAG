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

#include <time.h>
#include <cstdint>
#include "util/net.hpp"
#include "game.hpp"
#include "room.hpp"
#include "space.hpp"
#include "util/util.hpp"
#include <util/protocol/outgoing.hpp>


struct Handler {
    int socket;

    void onWebsocketUp(uint64_t id, int m_socket) {
        socket = m_socket;
        printf("Got websocket up\n");
        SocketSendBuffer sender(socket);
        protocol::outgoing::TestFrame frame;
        frame.number = 9999999999;
        frame.numbertwo = 42069;
        frame.signedint = -2;
        frame.sendTo(&sender);
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
    }
};


int main() {
    NetworkServer<Handler, Game> ns(3001);
    Game game;
    ns.spawnOff(16, &game);
    game.spawnOff(2);
    game.block();
}