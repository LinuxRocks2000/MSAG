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

#include "util/net.hpp"


struct Game { // container class

};


struct Handler {
    void onWebsocketUp(uint64_t id) {

    }

    void httpRequest(HTTPRequest request, HTTPResponse* response) {
        if (request.uri == "/") {
            response -> code = 200;
            response -> data = "<!DOCTYPE html><title>Information</title><p>You have reached an MSAG server. This is just as server; you'll"
            " need to get your client somewhere else and specify this server when you connect.</p>";
            response -> contentType = HTTP_CONTENT_TYPE_HTML;
        }
        else {
            response -> code = 404;
            response -> data = "File Not Found";
            response -> contentType = HTTP_CONTENT_TYPE_PLAINTEXT;
        }
    }
};


int main() {
    NetworkServer<Handler, Game> ns(3001);
    Game game;
    ns.spawnOff(16, &game);
    ns.block();
}