#pragma once
#include <functional>
#include <atomic>
#include <thread>

class IpcServer {
public:
    using RequestHandler = std::function<std::string(const std::string&)>;

    IpcServer(RequestHandler handler);
    ~IpcServer();
    void start();
    void stop();

private:
    void run();
    RequestHandler handler_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    int server_fd_;
};