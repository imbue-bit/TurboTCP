#include "TcpRelay.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>
#include <cstring>
#include <errno.h>

#define MAX_EVENTS 64
#define PIPE_BUF_SIZE 65536

TcpRelay::TcpRelay() : target_port_(0), listen_fd_(-1), epoll_fd_(-1), running_(false) {}

TcpRelay::~TcpRelay() { stop(); }

void TcpRelay::setTarget(const std::string& ip, int port) {
    target_ip_ = ip;
    target_port_ = port;
}

void TcpRelay::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void TcpRelay::applyExtremeOptions(int fd) {
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)); // 禁用 Nagle 算法
    setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, (char*)&flag, sizeof(int)); // 快速 ACK
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int)); // 允许重用地址
    int busy_poll = 50; // us
    setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL, (char*)&busy_poll, sizeof(int)); // Busy Pool
}

void TcpRelay::start(int local_port) {
    if (running_) return;
    if (target_ip_.empty() || target_port_ == 0) {
        std::cerr << "Target not set!" << std::endl;
        return;
    }
    local_port_ = local_port;
    running_ = true;
    worker_thread_ = std::thread(&TcpRelay::eventLoop, this);
    std::cout << "TcpReply Started on port " << local_port_ << " -> " << target_ip_ << ":" << target_port_ << std::endl;
}

void TcpRelay::stop() {
    running_ = false;
    if (listen_fd_ != -1) close(listen_fd_);
    if (worker_thread_.joinable()) worker_thread_.join();
}

void TcpRelay::getStats(long long& tx, long long& rx, int& conns) {
    tx = total_tx_bytes_.load();
    rx = total_rx_bytes_.load();
    conns = active_connections_.load();
}

struct ConnContext {
    int client_fd;
    int server_fd;
    int pipe_c_to_s[2];
    int pipe_s_to_c[2];
};

void TcpRelay::eventLoop() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    applyExtremeOptions(listen_fd_);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(local_port_);

    if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Oh no! Bind failed.");
        return;
    }
    listen(listen_fd_, 128);
    setNonBlocking(listen_fd_);

    epoll_fd_ = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);

    while (running_) {
        int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listen_fd_) {
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int client_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &len);
                if (client_fd < 0) continue;
                int server_fd = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in target_addr;
                target_addr.sin_family = AF_INET;
                target_addr.sin_port = htons(target_port_);
                inet_pton(AF_INET, target_ip_.c_str(), &target_addr.sin_addr);
                if (connect(server_fd, (struct sockaddr*)&target_addr, sizeof(target_addr)) < 0) {
                    close(client_fd);
                    close(server_fd);
                    continue;
                }
                setNonBlocking(client_fd);
                setNonBlocking(server_fd);
                applyExtremeOptions(client_fd);
                applyExtremeOptions(server_fd);
                ConnContext* ctx = new ConnContext();
                ctx->client_fd = client_fd;
                ctx->server_fd = server_fd;
                pipe(ctx->pipe_c_to_s);
                pipe(ctx->pipe_s_to_c);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = ctx;
                active_connections_++;
                std::thread([this, client_fd, server_fd, ctx]() {
                    int p_in[2], p_out[2];
                    pipe(p_in); pipe(p_out);
                    while (running_) {
                        ssize_t n = splice(client_fd, NULL, p_in[1], NULL, PIPE_BUF_SIZE, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
                        if (n > 0) {
                            splice(p_in[0], NULL, server_fd, NULL, n, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
                            total_tx_bytes_ += n;
                        } else if (n == 0) break;
                        n = splice(server_fd, NULL, p_out[1], NULL, PIPE_BUF_SIZE, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
                        if (n > 0) {
                            splice(p_out[0], NULL, client_fd, NULL, n, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
                            total_rx_bytes_ += n;
                        } else if (n == 0) break;
                        if (n < 0 && errno == EAGAIN) std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    
                    close(client_fd);
                    close(server_fd);
                    close(p_in[0]); close(p_in[1]);
                    close(p_out[0]); close(p_out[1]);
                    delete ctx;
                    active_connections_--;
                }).detach();
            }
        }
    }
    close(epoll_fd_);
}