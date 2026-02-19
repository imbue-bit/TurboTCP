#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include <sstream>
#include "Daemon.hpp"
#include "Common.hpp"
#include "IpcClient.hpp"

void print_help() {
    std::cout << "================================================\n"
              << " TcpTurbo - Extreme TCP Optimizer \n"
              << "================================================\n"
              << "Service Controls:\n"
              << "  start             : Start the background daemon\n"
              << "  stop              : Stop the daemon and revert changes\n"
              << "\nOptimization Controls:\n"
              << "  enable            : Apply extreme kernel-level tuning\n"
              << "  disable           : Revert to system default settings\n"
              << "  custom \"k=v,k2=v\" : Apply custom sysctl parameters\n"
              << "\Relay (Zero-Copy Acceleration):\n"
              << "  relay <ip> <port> : Start relay to target (Local port: " << DEFAULT_RELAY_LOCAL_PORT << ")\n"
              << "  relay-stop        : Stop the relay service\n"
              << "\nMonitoring:\n"
              << "  dashboard         : Live statistics and speed monitor\n"
              << "================================================\n";
}

void show_dashboard() {
    std::cout << "\033[2J\033[1;1H";
    long long last_relay_tx = 0;
    auto last_time = std::chrono::steady_clock::now();

    while (true) {
        std::string resp = send_command("GET_STATS");
        if (resp.find("STATS|") == 0) {
            std::string data = resp.substr(6);
            std::stringstream ss(data);
            std::string item;
            std::vector<std::string> v;
            while(std::getline(ss, item, ',')) v.push_back(item);

            if (v.size() >= 7) {
                auto now = std::chrono::steady_clock::now();
                double duration = std::chrono::duration<double>(now - last_time).count();
                
                long long current_tx = std::stoll(v[4]);
                double mbps = (duration > 0) ? (current_tx - last_relay_tx) / 1024.0 / 1024.0 * 8.0 / duration : 0;
                
                last_relay_tx = current_tx;
                last_time = now;

                std::cout << "\033[H";
                std::cout << "=== TcpTurbo Dashboard ===\n\n";
                
                std::cout << "[1] Global Kernel Stats\n";
                std::cout << "    In Segments:    " << v[0] << "\n";
                std::cout << "    Out Segments:   " << v[1] << "\n";
                std::cout << "    Retrans Rate:   " << std::fixed << std::setprecision(3) << std::stod(v[3]) << " %\n\n";

                std::cout << "[2] Extreme Relay Acceleration (Port " << DEFAULT_RELAY_LOCAL_PORT << ")\n";
                std::cout << "    Active Conns:   " << v[6] << "\n";
                std::cout << "    Traffic TX:     " << std::stoll(v[4]) / 1024 / 1024 << " MB\n";
                std::cout << "    Traffic RX:     " << std::stoll(v[5]) / 1024 / 1024 << " MB\n";
                std::cout << "    Current Speed:  \033[1;32m" << mbps << " Mbps\033[0m    \n\n";
                
                std::cout << "Press Ctrl+C to exit dashboard..." << std::endl;
            }
        } else {
            std::cout << "\rError: Cannot connect to daemon. Is it running?   " << std::flush;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "start") {
        if (geteuid() != 0) {
            std::cerr << "Error: Root privileges required to start daemon (for sysctl).\n";
            return 1;
        }
        Daemon::start();
    } 
    else if (cmd == "stop") {
        Daemon::stop();
    } 
    else if (cmd == "enable") {
        std::cout << "Result: " << send_command("ENABLE") << std::endl;
    } 
    else if (cmd == "disable") {
        std::cout << "Result: " << send_command("DISABLE") << std::endl;
    } 
    else if (cmd == "relay") {
        if (argc < 4) {
            std::cout << "Usage: tcp-turbo relay <target_ip> <target_port>\n";
            return 1;
        }
        std::string msg = "RELAY_START:" + std::string(argv[2]) + ":" + std::string(argv[3]);
        std::cout << "Result: " << send_command(msg) << std::endl;
    } 
    else if (cmd == "relay-stop") {
        std::cout << "Result: " << send_command("RELAY_STOP") << std::endl;
    }
    else if (cmd == "dashboard") {
        show_dashboard();
    } 
    else if (cmd == "custom") {
        if (argc < 3) return 1;
        std::cout << "Result: " << send_command("CUSTOM:" + std::string(argv[2])) << std::endl;
    }
    else {
        print_help();
    }

    return 0;
}