/* By Tyler Clarke
    This work is protected by the GNU GPLv3 license, which is attached in the root directory of the project.
    TLDR; don't make closed-source stuff with my code, don't lie about writing it yourself, don't license it anything other than GPLv3.
    you can sell derivative works if you want (as long as the source code is provided), but that would be dumb.
    
    This is the network server for MSAG. It's aggressively multithreaded and supports HTTP and WebSocket. It has no real security built in;
    use it behind a reverse proxy. That'll also probably save you some DDoS and malformed-request hell. Reverse proxies are cool.

    If you're lookin' to contribute, just ctrl+F for the word "TODO". There's a *lot* of TODO. Seriously. If your only contribution to the file is
    to add your name at the top, I'm not going to accept it. I'm looking at you, 1000D ;)
*/


// convenient but dumb header-only way to do it. later I might make it a fancy linked object.

#include <vector>
#include <sys/poll.h>
#include <mutex>
#include <atomic>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <cstdlib>
#include <pthread.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <string>
#include <vector>


// definitions for the parser states
#define HTTP_METHOD   0 // waiting state too.
#define HTTP_URI      1 // we're buffering until we receive the URI
#define HTTP_VERSION  2 // we're waiting for the HTTP version (anything but 1.1 will fail)
#define HTTP_HEADER_1 3 // we're getting the name of a header
#define HTTP_HEADER_2 4 // we're getting the value of a header
#define HTTP_CONTENT  5 // we're reading http content

// definitions for the http method
#define HTTP_METHOD_NONE  -1 // either there was an error, or this hasn't been set yet.
#define HTTP_METHOD_GET    0 // READ
#define HTTP_METHOD_POST   1 // UPDATE
#define HTTP_METHOD_PUT    2 // CREATE
#define HTTP_METHOD_DELETE 3 // DELETE
// I give you... RUCD!

// definitions for http request flags
#define HTTP_REQUEST_AUTHENTICATED 1 // Set if the Handler declares this request successfully authenticated (via token or whatever).
#define HTTP_REQUEST_UPGRADE_WS    2 // Set if the client wants to upgrade to WebSocket (all other upgrades will fail, as will malformed WebSocket upgrades)

// definitions for http response flags
#define HTTP_RESPONSE_UPGRADE_WS   1 // set if the ws upgrade will be accepted (the http responder will render the necessary headers from this).

// definitions for content types
#define HTTP_CONTENT_TYPE_PLAINTEXT  0
#define HTTP_CONTENT_TYPE_HTML       1
#define HTTP_CONTENT_TYPE_CSS        2
#define HTTP_CONTENT_TYPE_JAVASCRIPT 3

// definitions for HTTP upgrades
#define HTTP_UPGRADE_ERROR     0 // the client *wants* to upgrade, but is doing it wrong. This should fail 400.
// This may also be returned for valid upgrades that the server is not equipped to handle.
#define HTTP_UPGRADE_NONE      1 // we ain't upgrading, eh?
#define HTTP_UPGRADE_WEBSOCKET 2 // websockets
#define HTTP_UPGRADE_OTHER     3 // anything else
// it's certainly possible, but probably will never be necessary, to add other upgrade types


// convert an HTTP status code to the word it represents
const char* code2word(int code) {
    switch (code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "File Not Found";
        case 101: return "Switching Protocols";
    }
    return "I've Got Problems, OK?";
}

// convert an HTTP content type integer (see above) to content-type string
const char* contentTypeString(int contentType) {
    switch (contentType) {
        case HTTP_CONTENT_TYPE_HTML      : return "text/html";
        case HTTP_CONTENT_TYPE_CSS       : return "text/css";
        case HTTP_CONTENT_TYPE_JAVASCRIPT: return "application/javascript";
    }
    return "text/plain";
}


// utility function to check if a string starts with another string; strncmp was being pissy and I don't want to use std strings for everything (mainly because
// it's harder to control where they're allocated, and a lot of this program involves very fast allocation of small VLAs to stack)
bool startsWith(const char* string, const char* segment) {
    size_t len = strlen(segment);
    for (size_t i = 0; i < len; i ++) {
        if (string[i] != segment[i]) {
            return false;
        }
    }
    return true;
}

std::string lowerCase(std::string in) {
    for (size_t i = 0; i < in.size(); i ++) {
        if (in[i] >= 'A' && in[i] <= 'Z') { // could be optimized, but is it really necessary? see that random stackoverflow
            // post where that guy says it's not necessary
            in[i] -= 'A';
            in[i] += 'a';
        }
    }
    return in;
}


struct RingString { // ring-buffer of chars. mainly useful as an efficient FIFO pipe between socket readers and the asynchronous message processor
    char* buffer = NULL;
    size_t length = 0;
    size_t capacity = 0;
    size_t offset = 0;
    // ringstrings look like this in memory:
    // |hello world|. Nice, right?
    // the cool bit is, if we want to pop a byte off the back, we just increment offset.
    // h|ello world|.
    // now's the trippy bit: since length of this string is 10, but capacity is still 11, popping on another byte is simple: we increment length, don't change offset,
    // and put the new byte where that old byte was.
    // !|ello world|.
    // To read any byte from this, we add offset to the index and modulo by length. So in !|ello world|, index 1 is 2, and index 11 is (11 + 1) % 11 = 1, '!'. Simple!
    // If we need to fit more data than we *can*, we just reallocate and reset offset to 0. So if we pop on " Lorem", it becomes
    // |ello world! Lorem|. Nice and clean.
    // What if we instead want to push 'H' to the *start*? It's simple. We've got h|ello world|, we move offset back one and set the byte. 
    // |Hello world|. What if we want to push more than capacity to the start? Again, we reallocate, but this time we slap the old data at the *end* of the new buffer.
    // Easy!

    ~RingString() {
        if (buffer != NULL) {
            free(buffer);
        }
    }

    void _allocate(size_t size, int cpy) { // internal, DO NOT TOUCH!
        // cpy: -1=align old data to front, 1=align old data to back, 0=don't copy old data
        char* oldBuffer = buffer;
        buffer = (char*)malloc(size);
        if (cpy == -1) {
            for (size_t i = 0; i < length; i ++) {
                buffer[i] = oldBuffer[index(i)];
            }
        }
        else if (cpy == 1) {
            for (size_t i = 0; i < length; i ++) {
                buffer[i + size - length] = oldBuffer[index(i)];
            }
        }
        if (oldBuffer != NULL) {
            free(oldBuffer);
        }
        capacity = size;
    }

    inline size_t index(size_t virtualIndex) {
        return (virtualIndex + offset) % capacity;
    }

    void pushBack(const char* data, size_t dLength) {
        size_t oL = length;
        if (dLength + length > capacity) {
            _allocate(length + dLength, -1);
            offset = 0;
        }
        length += dLength;
        for (size_t i = 0; i < dLength; i ++) {
            buffer[index(i + oL)] = data[i];
        }
    }

    void pushBack(std::string data) {
        pushBack(data.c_str(), data.size());
    }

    void pushBack(char data) {
        if (length >= capacity) {
            _allocate(length + 1, -1);
            offset = 0;
        }
        buffer[index(length)] = data;
        length ++;
    }

    char popFront() {
        length --;
        char r = buffer[index(0)];
        offset ++;
        if (offset >= capacity) { // if we're pushing the end of the buffer
            offset = 0;
        }
        return r;
    }

    void popFront(char* buffer, size_t size) {
        for (size_t i = 0; i < size; i ++) {
            buffer[i] = popFront(); // todo: improve this with a dedicated memory copy or something
        }
    }

    std::string popFront(size_t size) { // todo: make this more efficient. maybe. not sure if that's possible. it probably is.
        std::string ret;
        ret.reserve(size);
        for (size_t i = 0; i < size; i ++) {
            ret += popFront();
        }
        return ret;
    }

    char operator[](size_t i) {
        return buffer[index(i)];
    }

    int checkFront(char stop) { // count up in the buffer until we
        // A) reach a character equal to stop, and return the index
        // B) reach the end of the buffer, and return -1.
        // this can be used in parser state machines to grab a word, or the like.
        for (size_t i = 0; i < length; i ++) {
            if (buffer[index(i)] == stop) {
                return i;
            }
        }
        return -1;
    }

    bool isAtNewLine() { // return if we're currently on a newline. this is useful for checking empty-lines.
        return (length >= 1 && buffer[offset] == '\n') || (length >= 2 && buffer[offset] == '\r' && buffer[offset + 1] == '\n');
    }

    void trimFront() {
        while ((length > 0) && (buffer[offset] == ' ' || buffer[offset] == '\n' || buffer[offset] == '\r')) {
            offset ++;
            length --;
        }
    }
};


template <size_t BufferSize=512>
struct SocketSendBuffer { // buffers sends on a socket, Nagle-style, to minimize syscalls.
    char buffer[BufferSize]; // very small buffer for now; increase size later if necessary (most messages will be under a kilobyte)
    int size = 0;
    int csocket;

    SocketSendBuffer(int s) {
        csocket = s;
    }

    void write(const char* data, size_t length) {
        if (length + size < BufferSize) {
            for (size_t i = 0; i < length; i ++) {
                buffer[size + i] = data[i];
            }
            size += length;
        }
        else { // if the buffer is at 510, and we're trying to write "Hello World" (length 11)
            size_t writeAmount = BufferSize - size - 1; // writeAmount is now 1
            write(data, writeAmount); // write the first segment of the data ('H' in the example)
            flush(); // flush the buffer (size is now 0, all data has been sent to the client)
            write(data + writeAmount, length - writeAmount); // length - writeAmount is 10: we're writing "ello World".

            // this will recurse sanely for absurdly long data.
        }
    }

    void write(const char* data) {
        write(data, strlen(data));
    }

    void write(std::string data) {
        write(data.c_str(), data.size());
    }

    void flush() { // always call this after you're done sending any frame of anything; it's never guaranteed what will be left in it.
        if (size != 0) { // if your data aligned perfectly with the buffer, the final flush call you MUST make will have nothing to flush. so just immediately return.
            send(csocket, buffer, size, 0);
            size = 0; // does NOT clean the buffer; make sure to pay attention to size at all times!
        }
    }
};


struct StringPair {
    std::string one;
    std::string two;
};


struct WebSocketFrame {};
struct HTTPRequest {
    int method = HTTP_METHOD_NONE; // see the top of this file.
    size_t size = 0; // if the content-length header isn't present, the data length will be assumed to be 0.
    std::string uri;
    std::vector<StringPair> headers; // any headers and cookies we don't have a flag for will be stored in these vectors.
    std::vector<StringPair> cookies;
    uint32_t flags = 0; // see the start of this file for applicable flags
    std::string content;

    bool header(std::string name, std::string value) { // check if a header is == a value (returns false if that header is not present)
        for (int i = 0; i < headers.size(); i ++) {
            if (headers[i].one == name) {
                return headers[i].two == value; // NOTE: cookies will fail with this because only the first Cookie header will be processed
                // (it's a short-circuit optimization). This is why we have dedicated cookie processing. Sorta. Er.
                // Ok, it's a TODO, but when we have dedicated cookie processing it'll work and this still won't. Yeah.
                // I'm not aware of any other headers that we'll have to worry about that allow duplication, so except for cookies
                // this is a useful optimization.
                // If you don't see it, don't worry; it's not *that* much of an optimization.
                // Just don't mess with this part of the code if you don't get how it's optimized.
            }
        }
        return false;
    }

    std::string header(std::string name) {
        for (int i = 0; i < headers.size(); i ++) {
            if (headers[i].one == name) {
                return headers[i].two;
            }
        }
        return "";
    }

    int getUpgrade() { // TODO: do upgrade processing *during* request processing to avoid all this loop overhead.
        // that's a really big TODO, actually, so let's make it four TODOs instead of one TODO. hee hee hee
        if (!header("connection", "upgrade")) {
            return HTTP_UPGRADE_NONE;
        }
        std::string h = header("upgrade");
        if (h == "") { // if we're getting nothing for the upgrade header, but the connection is set to upgrade (see above shortcircuit), this is a malformed request.
            return HTTP_UPGRADE_ERROR;
        }
        if (h == "websocket") {
            if (header("sec-websocket-version", "13")) { // TODO: check if we *really* have to verify a sec-websocket-key. I bet we don't, so I'm not adding it right now.
                return HTTP_UPGRADE_WEBSOCKET;
            }
            return HTTP_UPGRADE_ERROR; // it doesn't have a websocket version we can parse, this is a problem.
        }
        return HTTP_UPGRADE_OTHER; // usually we'll just fail on this as though it were HTTP_UPGRADE_ERROR, but it's nice to have the distinction
    }
};
struct HTTPResponse {
    int code;
    int flags = 0; // see the top of this file
    std::vector<StringPair> headers; // anything that can't be handled with flags will go in these.
    std::vector<StringPair> cookies;
    std::string data;
    int contentType = HTTP_CONTENT_TYPE_PLAINTEXT;

    HTTPResponse(int c, std::string d) {
        code = c;
        data = d;
    }

    HTTPResponse() {
        
    }

    void sendTo(SocketSendBuffer<> socket, HTTPRequest* initiator = NULL) { // pass in the http request that initiated this, if you feel like it. You don't _need_ to.
        socket.write("HTTP/1.1 ");
        socket.write(std::to_string(code));
        socket.write(" ");
        socket.write(code2word(code));
        socket.write("\r\n");

        socket.write("Server: MSAG-Server\r\n");

        for (size_t i = 0; i < headers.size(); i ++) {
            socket.write(headers[i].one);
            socket.write(": ");
            socket.write(headers[i].two);
            socket.write("\r\n");
        }
        // TODO: cookies.

        socket.write("Content-Length: ");
        socket.write(std::to_string(data.size()));
        socket.write("\r\n");
        socket.write("Content-Type: ");
        socket.write(contentTypeString(contentType));
        socket.write("\r\n");
        socket.write("\r\n"); // that empty line
        socket.write(data);

        socket.flush();
    }
};

template <typename Handler, typename DataArgument>
struct ClientInternal {
    std::mutex me_mutex;
    RingString buffer;
    int socket;
    Handler handler;
    int state = HTTP_METHOD; // see the top of this file
    HTTPRequest httpBuffer;
    WebSocketFrame wsBuffer;
    std::string headerName;
    uint64_t id;

    ClientInternal(int sock, uint64_t ident) {
        socket = sock;
        id = ident;
    }

    void badRequest(std::string body = "The data you sent me... eet eez ze BAD! Please to improving zis datas!\n") {
        HTTPResponse error (400, body);
        error.sendTo(socket);
        shutdown(socket, SHUT_RDWR); // the "graceful shutdown"; sends a FIN and notifies the kernel that this connection is over
        close(socket); // destroy the file descriptor, we aren't going to wait for any more data
    }

    void gotData() { // this behemoth processes an HTTP or WebSocket frame.
        // Interestingly, this is approximately what an async function looks like when the compiler expands it, although this one
        // uses some trickery to avoid the heavier parts of async state change (it won't exit until the entire buffer is processed,
        // and will exit immediately when it is)
        // I don't intend to use this absolutely disgusting expanded-async paradigm beyond this function. State machines will be in use,
        // but not *big, ugly* ones like this. Nice pretty ones. With flowers. And maybe some rainbows.
        if (state == HTTP_METHOD) {
            int up = buffer.checkFront(' ');
            if (up != -1) {
                if (up < 7) { // anything larger than this is an error, we need to just break off immediately.
                    char data[up + 1];
                    data[up] = 0; // NULL terminator (for strcmp)
                    buffer.popFront(data, up);
                    buffer.popFront(); // purge the space
                    if (strcmp(data, "GET") == 0) {
                        httpBuffer.method = HTTP_METHOD_GET;
                    }
                    else if (strcmp(data, "POST") == 0) {
                        httpBuffer.method = HTTP_METHOD_POST;
                    }
                    else if (strcmp(data, "PUT") == 0) {
                        httpBuffer.method = HTTP_METHOD_PUT;
                    }
                    else if (strcmp(data, "DELETE") == 0) {
                        httpBuffer.method = HTTP_METHOD_DELETE;
                    }
                    else {
                        badRequest();
                    }
                    state = HTTP_URI;
                }
                else {
                    badRequest();
                }
            }
        }
        // no else-if, in case we made it past the HTTP_METHOD stage and there's a uri in buffer already.
        if (state == HTTP_URI) {
            int up = buffer.checkFront(' ');
            if (up != -1) {
                httpBuffer.uri = buffer.popFront(up);
                buffer.popFront(); // purge the space
                state = HTTP_VERSION;
            }
        }
        if (state == HTTP_VERSION) {
            int up = buffer.checkFront('\n');
            if (up != -1) {
                if (up >= 8) {
                    char buff[up];
                    buffer.popFront(buff, up);;
                    if (startsWith(buff, "HTTP/1.1")) { // use strncmp here to ignore \r (by only parsing the first 8 bytes)
                        state = HTTP_HEADER_1;
                        buffer.popFront(); // purge the newline
                    }
                    else {
                        badRequest();
                    }
                }
                else {
                    badRequest();
                }
            }
        } // TODO: size limits. seriously.
        while (state == HTTP_HEADER_1 || state == HTTP_HEADER_2) {
            if (state == HTTP_HEADER_1) {
                if (buffer.isAtNewLine()) {
                    if (buffer.popFront() == '\r') {
                        buffer.popFront(); // if we're handling a CRLF, do an extra pop to get that LF.
                    }
                    state = HTTP_CONTENT;
                }
                else {
                    int up = buffer.checkFront(':');
                    if (up == -1) {
                        break;
                    }
                    else{
                        headerName = lowerCase(buffer.popFront(up));
                        buffer.popFront();
                        state = HTTP_HEADER_2;
                    }
                }
            }
            if (state == HTTP_HEADER_2) {
                buffer.trimFront();
                int up = buffer.checkFront('\n');
                if (up == -1) {
                    break;
                }
                else {
                    std::string headerData = buffer.popFront(up);
                    buffer.popFront(); // pop out the \n
                    if (headerName == "content-length") {
                        httpBuffer.size = stoi(headerData);
                    }
                    else {
                        httpBuffer.headers.push_back(StringPair {
                            .one = headerName,
                            .two = headerData
                        });
                    }
                    // TODO: other header processing
                    state = HTTP_HEADER_1;
                }
            }
        }
        if (state == HTTP_CONTENT) {
            if (buffer.length >= httpBuffer.size) {
                httpBuffer.content = buffer.popFront(httpBuffer.size);
                // this http request is finished. let's send it to the handler, and clear the HTTP buffer.
                state = HTTP_METHOD;
                handleRequest(httpBuffer);
                httpBuffer = HTTPRequest{};               
            }
        }
    }

    void handleRequest(HTTPRequest request) {
        HTTPResponse response;
        int upgrade = request.getUpgrade();
        if (upgrade == HTTP_UPGRADE_ERROR || upgrade == HTTP_UPGRADE_OTHER) {
            response.code = 400;
            response.data = "Bad Upgrade!";
        }
        else { // if the server doesn't have other processing routines for this request, just yield to the handler
            handler.httpRequest(request, &response);
        }
        response.sendTo(socket, &request);
    }
};


template <typename ClientHandler, typename DataArgument>
struct NetworkServer { // simple HTTP/WebSocket server designed for use behind a reverse proxy. Thread-pooled and polling;
    // spawns a preconfigured number of threads and polls any number of clients on them. Manages pollfds with std::vector.
    // Has a *separate* vector of Clients, aligned with the fds vector for very fast index matching.
    // The first index of the fds vector is the accepting socket, so all indexes should be bumped backwards one when translating to the clients vector.
    std::vector<struct pollfd> fds;
    std::vector<ClientInternal<ClientHandler, DataArgument>*> clients;
    std::atomic<uint64_t> client_id = 0; // unique client id incrementer. new clients that pop into the server get this id, and then it's incremented.
    // When poll breaks in any polling thread, it should iterate over the entire array checking revents to see if there's anything to handle,
    // and them the internal std::mutexes to see if another thread is handling them. If not, lock the mutex and set revents to 0, so another thread won't pull that data,
    // and handle whatever data went through the pipe.
    // This way, the server doesn't use much more CPU time than it absolutely needs (the extra iteration + mutex checks won't be very expensive, even at 10000 clients -
    // most clients won't waste more than a few cycles)
    // and also ships events to the custom ClientHandlers at lightnin' speed.
    // note: to avoid a particularly annoying race condition where clients disconnect right after the poll (and right before the recv), locking the thread,
    // use non-blocking IO for every socket handled by this class. Also, enable nodelay (disable Nagle's algorithm): since this is meant for an online game, we can't have
    // dear Nagle slowing down our packets.
    //
    // In case of disconnect/connect, we need some sane way to get the updated record to every thread. Interrupting the polls will produce *ridiculous* race conditions,
    // and synchronizing will destroy server performance, and we can't just deal with it
    std::atomic<uint64_t> masterAge = 1; // set to 1 so every thread will copy immediately
    std::mutex editMutex; // in the current design, this locks whenever copying from or changing the vectors. Improve later.

    DataArgument* arg = NULL; // passed to handlers after spawning threads
    // Since there's no client object to contain the mutex automatically, we have to explicitly specify a mutex for the server
    std::mutex serverMutex;

    int threadCount = 0; // if we ever need this, it'll already be ready
    // does nothing at the moment
    // shrug
public:
    NetworkServer(int port) {
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr)); // make sure the address is zeroed: costs us nothing (well, something, but not much of it)
        // and potentially saves us from a REALLY nasty bug.
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        int t = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) != 0) {
            printf("Couldn't set the server socket to reuseaddr | reuseport.\n");
            perror("setsockopt");
            exit(1);
        }
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
            perror("bind");
            exit(1);
        }
        if (listen(serverSocket, 32) != 0) { // 32 is a nice round number.
            perror("listen");
            exit(1);
        }
        if (!nonblock(serverSocket)) {
            printf("Server socket nonblocking failed. This is not recoverable. Exiting now.\n");
            exit(1);
        }
        fds.push_back({
            .fd = serverSocket,
            .events = POLLIN, // POLLHUP and POLLNVAL are automatically listened.
            .revents = 0
        });
    }

    static bool nonblock(int socket) {
        // set nonblocking IO mode. This can be used for (bad) spinlocking, but in this specific case we're just using it to avoid annoying race conditions with poll.
        fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, NULL) | O_NONBLOCK);
        // disable nagle's algorithm. Nagle's algorithm buffers TCP packets until they're large enough for the sender to consider them worthy of a whole tcp exchange;
        // this significantly improves network efficiency, but also adds significant latency.
        int flag = 1;
        if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) != 0) {
            printf("Couldn't set a socket to nonblocking mode\n");
            perror("setsockopt");
            return false;
        }
        return true;
    }

    static void* child(void* _me) {
        NetworkServer* self = (NetworkServer*)_me;
        std::vector<struct pollfd> fds; // thread-local cache
        std::vector<ClientInternal<ClientHandler, DataArgument>*> clients; // copied whenever it gets old
        uint64_t age = 0;
        while (true) {
            if (age != self -> masterAge) {
                self -> editMutex.lock();
                fds = self -> fds; // NOTE: this is a ridiculous solution. As we approach ten thousand clients, this will copy FAR too much data every join
                clients = self -> clients; // to possibly be scalable. If we want this server to scale, we need to fix this!
                // Fortunately, this doesn't affect rw speeds, it's just join/disconnect.
                // Message passing?
                age = self -> masterAge; // update the data age so we don't do this again next time.
                self -> editMutex.unlock();
            }

            poll(fds.data(), fds.size(), -1); // poll on everything

            for (size_t i = 0; i < fds.size(); i ++) {
                int clientIndex = i - 1; // because the first element of fds is the server socket
                if (fds[i].revents != 0 && (i == 0 ? self -> serverMutex.try_lock() : clients[clientIndex] -> me_mutex.try_lock())) { 
                    if (fds[i].revents & POLLIN) { // the socket has data to read
                        if (i == 0) { // POLLIN on the server socket (which is always the first file descriptor in that array) is an incoming client.
                            struct sockaddr_in addrbuf;
                            socklen_t addrbufsize = sizeof(addrbuf);
                            int client = accept(fds[0].fd, (struct sockaddr*)&addrbuf, &addrbufsize);
                            if (client == -1) {
                                printf("Got a phantom client; this is not critical.\n");
                                continue;
                            }
                            if (!nonblock(client)) {
                                printf("Couldn't nonblock a client socket. Will not continue connection.\n");
                                shutdown(client, SHUT_RDWR);
                                close(client);
                                continue;
                            }

                            self -> editMutex.lock();

                            self -> fds.push_back({
                                .fd = client,
                                .events = POLLIN, // lissen fer data
                                .revents = 0
                            });
                            self -> masterAge ++;
                            ClientInternal<ClientHandler, DataArgument>* clientObj = new ClientInternal<ClientHandler, DataArgument>(client, (uint64_t)self -> client_id); // allocate to heap
                            self -> client_id ++;
                            // because mutexes and ringstrings don't like being moved around the stack
                            self -> clients.push_back(clientObj);

                            self -> editMutex.unlock();

                            printf("Client connected.\n");
                        }
                        else { // we're handling a client, they probably sent us something. maybe it was yummy!
                            char yummy[1024]; // most messages this server receives will be 1k or less
                            int bytesRead = recv(fds[i].fd, yummy, 1024, 0);
                            if (bytesRead == 0) { // disconnect case
                            // TODO: make this not copy/pasted.
                                self -> editMutex.lock(); // lock the master list
                                if (age == self -> masterAge) { // check that our data is old enough - if it's not, we aren't qualified to delete an entry
                                    self -> fds[i] = self -> fds[self -> fds.size() - 1]; // swap remove the file descriptor data
                                    self -> fds.pop_back();
                                    delete self -> clients[clientIndex]; // delete the client
                                    self -> clients[clientIndex] = self -> clients[self -> clients.size() - 1]; // swap the pointer out of the array
                                    self -> clients.pop_back();
                                    printf("Client disconnected.\n");
                                }
                                self -> masterAge ++; // increment the data age. because it's old data now. yep.
                                self -> editMutex.unlock();
                            }
                            else if (bytesRead == -1) { // somehow, an error occurred.
                                printf("Some kind o' weird socket error occurred.\n");
                                perror("recv");
                            }
                            else {
                                clients[clientIndex] -> buffer.pushBack(yummy, bytesRead);
                                clients[clientIndex] -> gotData();
                            }
                        }
                    }
                    if ((fds[i].revents & POLLHUP) || (fds[i].revents & POLLNVAL)) { // the socket was disconnected
                        self -> editMutex.lock(); // lock the master list (do this before the check, to avoid race conditions)
                        if (age == self -> masterAge) { // check that our data is old enough - if it's not, we aren't qualified to delete an entry,
                            // so we immediately unlock.
                            self -> fds[i] = self -> fds[self -> fds.size() - 1]; // swap remove the file descriptor data
                            self -> fds.pop_back();
                            delete self -> clients[clientIndex]; // delete the client
                            self -> clients[clientIndex] = self -> clients[self -> clients.size() - 1]; // swap the pointer out of the array
                            self -> clients.pop_back();
                            printf("Client disconnected.\n");
                        }
                        self -> masterAge ++; // increment the data age. because it's old data now. yep.
                        self -> editMutex.unlock();
                    }
                    if (i == 0) {
                        self -> serverMutex.unlock();
                    }
                    else {
                        clients[clientIndex] -> me_mutex.unlock();
                    }
                }
                else if (i == 0) {
                    self -> serverMutex.lock(); // wait until the accepting thread is done wit iz biznus
                    self -> serverMutex.unlock();
                }
            }
        }
        return NULL;
    }

    void spawnOff(int numThreads, DataArgument* argument) {
        pthread_t buffer;
        threadCount += numThreads;
        for (int i = 0; i < numThreads; i ++) {
            pthread_create(&buffer, NULL, NetworkServer::child, this);
            pthread_detach(buffer); // detach the thread; we will not join it
        }
    }

    void block() { // block the main thread on waiting for IO
        // this is convenient if, in the future, we decide to use this nice distributed poll implementation for timing the gameloop.
        // and also for testing things
        threadCount ++;
        NetworkServer::child((void*)this); // run the code for any thread of this server - on the main thread. possible danger, will robinson.
    }
};