#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include "Common.hpp"

class TcpRelay {
public:
    TcpRelay();
    ~TcpRelay();
    void setTarget(const std::string& ip, int port);
    void start(int local_port = DEFAULT_RELAY_LOCAL_PORT);
    void stop();
    void getStats(long long& tx, long long& rx, int& conns);

private:
    void eventLoop();
    void setNonBlocking(int fd);
    void applyExtremeOptions(int fd);
    std::string target_ip_;
    int target_port_;
    int local_port_;
    int listen_fd_;
    int epoll_fd_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::atomic<long long> total_tx_bytes_{0};
    std::atomic<long long> total_rx_bytes_{0};
    std::atomic<int> active_connections_{0};
};