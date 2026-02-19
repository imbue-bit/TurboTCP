#include "IpcServer.hpp"
#include "Common.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <vector>

IpcServer::IpcServer(RequestHandler handler) : handler_(handler), running_(false), server_fd_(-1) {}

IpcServer::~IpcServer() {
    stop();
}

void IpcServer::start() {
    if (running_) return;
    running_ = true;
    server_thread_ = std::thread(&IpcServer::run, this);
}

void IpcServer::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    unlink(SOCKET_PATH);
}

void IpcServer::run() {
    unlink(SOCKET_PATH);

    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        perror("socket error");
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        return;
    }
    
    chmod(SOCKET_PATH, 0600); 

    if (listen(server_fd_, 5) == -1) {
        perror("listen error");
        return;
    }

    while (running_) {
        int client_fd = accept(server_fd_, NULL, NULL);
        if (client_fd == -1) {
            if (running_) perror("accept error");
            continue;
        }

        char buffer[1024] = {0};
        int rc = read(client_fd, buffer, sizeof(buffer));
        if (rc > 0) {
            std::string response = handler_(std::string(buffer, rc));
            write(client_fd, response.c_str(), response.length());
        }
        close(client_fd);
    }
}