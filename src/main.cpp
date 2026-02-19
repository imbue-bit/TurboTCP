#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <iomanip>
#include "Daemon.hpp"
#include "Common.hpp"

std::string send_command(const std::string& cmd);

void print_help() {
    std::cout << "TcpTurbo - Extreme TCP Optimizer\n"
              << "Usage:\n"
              << "  tcp-turbo start       : Start the background daemon\n"
              << "  tcp-turbo stop        : Stop the daemon\n"
              << "  tcp-turbo enable      : Enable extreme optimizations\n"
              << "  tcp-turbo disable     : Restore default settings\n"
              << "  tcp-turbo custom \"k=v\" : Apply custom sysctl params\n"
              << "  tcp-turbo dashboard   : Live statistics view\n";
}

void show_dashboard() {
    std::cout << "\033[2J\033[1;1H";
    std::cout << "TcpTurbo Dashboard (Ctrl+C to exit)\n";
    std::cout << "--------------------------------------\n";
    long long last_out = 0;
    while (true) {
        std::string resp = send_command("GET_STATS");
        if (resp.find("STATS|") == 0) {
            std::string data = resp.substr(6);
            std::stringstream ss(data);
            std::string segment;
            std::vector<std::string> vals;
            while(std::getline(ss, segment, ',')) vals.push_back(segment);
            
            if (vals.size() >= 4) {
                long long out = std::stoll(vals[1]);
                double speed = (last_out == 0) ? 0 : (out - last_out);
                last_out = out;

                std::cout << "\033[4;1H";
                std::cout << "Total In Segs:   " << vals[0] << "\n";
                std::cout << "Total Out Segs:  " << vals[1] << "\n";
                std::cout << "Retrans Segs:    " << vals[2] << "\n";
                std::cout << "Retrans Rate:    " << std::fixed << std::setprecision(4) << vals[3] << " %\n";
                std::cout << "Sending Speed:   " << speed << " segs/sec\n";
            }
        } else {
            std::cout << "Error connecting to daemon.\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
            std::cerr << "Error: Must run as root to start service.\n";
            return 1;
        }
        Daemon::start();
    } else if (cmd == "stop") {
        Daemon::stop();
    } else if (cmd == "enable") {
        std::cout << send_command("ENABLE") << std::endl;
    } else if (cmd == "disable") {
        std::cout << send_command("DISABLE") << std::endl;
    } else if (cmd == "custom") {
        if (argc < 3) {
            std::cout << "Usage: tcp-turbo custom \"net.core.wmem_max=99999\"\n";
            return 1;
        }
        std::string params = "CUSTOM:" + std::string(argv[2]);
        std::cout << send_command(params) << std::endl;
    } else if (cmd == "dashboard") {
        show_dashboard();
    } else {
        print_help();
    }

    return 0;
}