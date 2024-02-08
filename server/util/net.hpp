// convenient header-only way to do it. later I might make it a fancy linked object.


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


struct RingString { // ring-buffer of chars.
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

    void _allocate(size_t size) { // internal, DO NOT TOUCH!
        char* oldBuffer = buffer;
        capacity = size;
        buffer = (char*)malloc(size);
        if (oldBuffer != NULL) {
            free(oldBuffer);
        }
    }

    inline size_t index(size_t virtualIndex) {
        return (virtualIndex + offset) % length;
    }

    void pushBack(const char* data, size_t dLength) {
        if (dLength + length > capacity) {
            _allocate(length + dLength);
            offset = 0;
        }
        for (size_t i = 0; i < dLength; i ++) {

        }
        length += dLength;
    }

    void pushBack(char data) {
        if (length + 1 > capacity) {
            _allocate(length + 1);
            offset = 0;
        }
        buffer[index(length)] = data;
        length ++;
    }
};


template <typename Handler, typename DataArgument>
struct ClientInternal {
    std::mutex me_mutex;
    RingString buffer;
    int socket;
    Handler handler;
    ClientInternal(int sock) {
        socket = sock;
    }
};


template <typename ClientHandler, typename DataArgument>
struct NetworkServer { // simple HTTP/WebSocket server designed for use behind a reverse proxy. Thread-pooled and polling;
    // spawns a preconfigured number of threads and polls any number of clients on them. Manages pollfds with std::vector.
    // Has a *separate* vector of Clients for very fast index matching.
    // The first index of the fds vector is the accepting socket, so all indexes should be bumped backwards one when translating to the clients vector.
    std::vector<struct pollfd> fds;
    std::vector<ClientInternal<ClientHandler, DataArgument>*> clients;
    std::atomic<uint64_t> client_id = 0; // unique client id incrementer. new clients that pop into the server 
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
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
            perror("bind");
            exit(1);
        }
        if (listen(serverSocket, 32) != 0) { // 32 is a nice round number.
            perror("listen");
            exit(1);
        }
        nonblock(serverSocket);
        fds.push_back({
            .fd = serverSocket,
            .events = POLLIN, // POLLHUP and POLLNVAL are automatically listened.
            .revents = 0
        });
    }

    static void nonblock(int socket) {
        // set nonblocking IO mode. This can be used for (bad) spinlocking, but in this specific case we're just using it to avoid annoying race conditions with poll.
        fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, NULL) | O_NONBLOCK);
        // disable nagle's algorithm. Nagle's algorithm buffers TCP packets until they're large enough for the sender to consider them worthy of a whole tcp exchange;
        // this significantly improves network efficiency, but also adds significant latency.
        int flag = 1;
        setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
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
                printf("Now got %d clients\n", clients.size());
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
                            nonblock(client);

                            self -> editMutex.lock();

                            self -> fds.push_back({
                                .fd = client,
                                .events = POLLIN, // lissen fer data
                                .revents = 0
                            });
                            self -> masterAge ++;
                            ClientInternal<ClientHandler, DataArgument>* clientObj = new ClientInternal<ClientHandler, DataArgument>(client); // allocate to heap
                            // because mutexes and ringstrings don't like being moved around the stack
                            self -> clients.push_back(clientObj);

                            self -> editMutex.unlock();

                            printf("Client connected.\n");
                        }
                        else { // we're handling a client, they probably sent us something. maybe it was yummy!
                            char yummy[1024]; // most messages this server receives will be 1k or less
                            int bytesRead = recv(fds[i].fd, yummy, 1024, 0);
                            if (bytesRead == 0) { // disconnect case
                                self -> editMutex.lock(); // lock the master list
                                if (age == self -> masterAge) { // check that our data is old enough - if it's not, we aren't qualified to delete an entry
                                    self -> fds[i] = self -> fds[self -> fds.size() - 1]; // swap remove the file descriptor data
                                    self -> fds.pop_back();
                                    delete self -> clients[clientIndex]; // delete the client
                                    self -> clients[clientIndex] = self -> clients[self -> clients.size() - 1]; // swap the pointer out of the array
                                    self -> clients.pop_back();
                                }
                                self -> masterAge ++; // increment the data age. because it's old data now. yep.
                                self -> editMutex.unlock();
                                printf("Client disconnected.\n");
                            }
                            else if (bytesRead == -1) { // somehow, an error occurred. This is probably because the socket was barren or something?

                            }
                            else {
                                printf("data: %s\n", yummy);
                            }
                        }
                    }
                    if ((fds[i].revents & POLLHUP) || (fds[i].revents & POLLNVAL)) { // the socket was disconnected
                        printf("disconnect\n");
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