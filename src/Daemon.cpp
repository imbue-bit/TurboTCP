#include "Daemon.hpp"
#include "SystemTuner.hpp"
#include "StatsCollector.hpp"
#include "IpcServer.hpp"
#include "Common.hpp"
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <iostream>

static SystemTuner g_tuner;
static StatsCollector g_stats;

std::string Daemon::handleRequest(const std::string& req) {
    if (req == "ENABLE") {
        g_tuner.enableOptimization();
        return "OK|Optimizations Enabled";
    } else if (req == "DISABLE") {
        g_tuner.disableOptimization();
        return "OK|Optimizations Disabled";
    } else if (req.rfind("CUSTOM:", 0) == 0) {
        // 格式 CUSTOM:key=value,key2=value2
        std::string payload = req.substr(7);
        std::map<std::string, std::string> params;
        std::stringstream ss(payload);
        std::string item;
        while (std::getline(ss, item, ',')) {
            size_t del = item.find('=');
            if (del != std::string::npos) {
                params[item.substr(0, del)] = item.substr(del + 1);
            }
        }
        g_tuner.applyCustom(params);
        return "OK|Custom Applied";
    } else if (req == "GET_STATS") {
        auto stats = g_stats.collect();
        std::stringstream ss;
        ss << stats.in_segs << "," << stats.out_segs << "," 
           << stats.retrans_segs << "," << stats.retrans_rate;
        return "STATS|" + ss.str();
    } else if (req == "STOP") {
        return "OK|Stopping";
    } else if (req == "PING") {
        return "PONG";
    }
    return "ERR|Unknown Command";
}

void Daemon::runService() {
    IpcServer server(handleRequest);
    server.start();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Daemon::start() {
    if (access(PID_FILE, F_OK) != -1) {
        std::cerr << "Daemon already running (Check " << PID_FILE << ")" << std::endl;
        return;
    }

    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // 父进程退出

    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork(); // 二次 fork 防止获取终端控制权
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    std::ofstream pid_file(PID_FILE);
    pid_file << getpid();
    pid_file.close();

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    runService();
}

void Daemon::stop() {
    std::ifstream pid_file(PID_FILE);
    int pid;
    if (pid_file >> pid) {
        kill(pid, SIGTERM);
        unlink(PID_FILE);
        std::cout << "Daemon stopped." << std::endl;
    } else {
        std::cerr << "Could not read PID file." << std::endl;
    }
}