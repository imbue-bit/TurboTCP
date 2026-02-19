#pragma once
#include <string>

class Daemon {
public:
    static void start();
    static void stop();
private:
    static void runService();
    static std::string handleRequest(const std::string& req);
};