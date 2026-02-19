#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <chrono>
#include <iomanip>

namespace TurboTCP {
    class Logger {
    public:
        static void log(const std::string& msg) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::cout << "[" << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "] " << msg << std::endl;
        }
        
        static void error(const std::string& msg) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cerr << "[ERROR] " << msg << std::endl;
        }

    private:
        static std::mutex mutex_;
    };
    inline std::mutex Logger::mutex_;
}