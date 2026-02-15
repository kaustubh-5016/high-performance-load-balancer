// load_balancer.cpp
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

constexpr int MAX_EVENTS = 1024;
constexpr int BUFFER_SIZE = 4096;

struct Backend {
    std::string ip;
    int port;
};

std::vector<Backend> backends = {
    {"127.0.0.1", 9001},
    {"127.0.0.1", 9002}
};

int current_backend = 0;

int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int create_server_socket(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

    set_non_blocking(server_fd);
    return server_fd;
}

int connect_to_backend() {
    Backend& backend = backends[current_backend];
    current_backend = (current_backend + 1) % backends.size();

    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_non_blocking(backend_fd);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(backend.port);
    inet_pton(AF_INET, backend.ip.c_str(), &addr.sin_addr);

    connect(backend_fd, (sockaddr*)&addr, sizeof(addr));
    return backend_fd;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./lb <port>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    int server_fd = create_server_socket(port);

    int epoll_fd = epoll_create1(0);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    std::unordered_map<int, int> connection_map;

    epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                // Accept new client
                int client_fd = accept(server_fd, nullptr, nullptr);
                set_non_blocking(client_fd);

                int backend_fd = connect_to_backend();

                connection_map[client_fd] = backend_fd;
                connection_map[backend_fd] = client_fd;

                epoll_event client_event{};
                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);

                epoll_event backend_event{};
                backend_event.events = EPOLLIN;
                backend_event.data.fd = backend_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, backend_fd, &backend_event);

            } else {
                char buffer[BUFFER_SIZE];
                int bytes = recv(fd, buffer, BUFFER_SIZE, 0);

                if (bytes <= 0) {
                    int peer = connection_map[fd];
                    close(fd);
                    close(peer);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, peer, nullptr);
                    connection_map.erase(fd);
                    connection_map.erase(peer);
                } else {
                    int peer = connection_map[fd];
                    send(peer, buffer, bytes, 0);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
