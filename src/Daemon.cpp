#include "Daemon.hpp"
#include "SystemTuner.hpp"
#include "StatsCollector.hpp"
#include "TcpRelay.hpp"
#include "IpcServer.hpp"
#include "Common.hpp"
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>

static SystemTuner g_tuner;
static StatsCollector g_stats;
static TcpRelay g_relay;

std::string Daemon::handleRequest(const std::string& req) {
    if (req == "ENABLE") {
        g_tuner.enableOptimization();
        return "OK|Kernel Optimizations Enabled";
    } 
    else if (req == "DISABLE") {
        g_tuner.disableOptimization();
        g_relay.stop();
        return "OK|All Optimizations Disabled & Relay Stopped";
    } 
    else if (req.find("RELAY_START:") == 0) {
        // format: RELAY_START:1.2.3.4:80
        try {
            std::string payload = req.substr(12);
            size_t del = payload.find(':');
            if (del != std::string::npos) {
                std::string ip = payload.substr(0, del);
                int port = std::stoi(payload.substr(del + 1));
                g_relay.stop(); 
                g_relay.setTarget(ip, port);
                g_relay.start(DEFAULT_RELAY_LOCAL_PORT);
                return "OK|Relay Active: localhost:" + std::to_string(DEFAULT_RELAY_LOCAL_PORT) + " -> " + ip + ":" + std::to_string(port);
            }
        } catch (...) { return "ERR|Invalid Relay Parameters"; }
        return "ERR|Format Error";
    } 
    else if (req == "RELAY_STOP") {
        g_relay.stop();
        return "OK|Relay Service Stopped";
    } 
    else if (req == "GET_STATS") {
        auto stats = g_stats.collect();
        long long tx, rx;
        int conns;
        g_relay.getStats(tx, rx, conns);
        
        std::stringstream ss;
        // format: STATS|in,out,retrans,rate,relay_tx,relay_rx,active_conns
        ss << "STATS|" << stats.in_segs << "," << stats.out_segs << "," 
           << stats.retrans_segs << "," << stats.retrans_rate << ","
           << tx << "," << rx << "," << conns;
        return ss.str();
    } 
    else if (req == "PING") {
        return "PONG";
    }
    else if (req == "STOP") {
        g_tuner.disableOptimization();
        g_relay.stop();
        return "OK|Daemon Shuting Down";
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
    if (access(PID_FILE, F_OK) == 0) {
        std::cerr << "TcpTurbo daemon is already running. (PID file exists)\n";
        return;
    }

    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); 

    if (setsid() < 0) exit(EXIT_FAILURE);

    // 忽略 SIGHUP 防止终端关闭导致进程退出
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

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
        // 先通过 IPC 发送停止指令尝试优雅的退出
        send_command("STOP");
        kill(pid, SIGTERM);
        unlink(PID_FILE);
        unlink(SOCKET_PATH);
        std::cout << "TcpTurbo daemon (PID " << pid << ") has been stopped.\n";
    } else {
        std::cout << "Daemon is not running.\n";
    }
}