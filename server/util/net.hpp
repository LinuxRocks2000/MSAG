#include <vector>
#include <sys/poll.h>
#include <mutex>
#include <atomic>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>



template <typename ClientHandler>
class NetworkServer { // simple HTTP/WebSocket server designed for use behind a reverse proxy. Thread-pooled and polling;
    // spawns a preconfigured number of threads and polls any number of clients on them. Manages pollfds with std::vector.
    // Has a *separate* vector of Clients for very fast index matching.
    // The first index of the fds vector is the accepting socket, so all indexes should be bumped backwards one when translating to the clients vector.
    std::vector<struct pollfd> fds;
    std::vector<ClientInternal<ClientHandler>> clients;
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
    // In the disconnect case (POLLNVAL or POLLHUP), the socket will be swap-removed whenever it's possible to lock the mutex at the front and the mutex
    // of the socket to be removed at the front mutex until whoever's touching that is done.
    // In the 
public:
    NetworkServer(int port) {
        struct sockaddr_in server_addr;
        memset(&serverAddr, 0, sizeof(server_addr)); // make sure the address is zeroed: costs us nothing (well, something, but not much of it)
        // and potentially saves us from a REALLY nasty bug.
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (bind(serverSocket, &serverAddr, sizeof(server_addr)) != 0) {
            perror("bind");
            exit(1);
        }
        if (listen(serverSocket, 32) != 0) { // 32 is a nice round number.
            perror("listen");
            exit(1);
        }
        fds.push_back()
    }
}