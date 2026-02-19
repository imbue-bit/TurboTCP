#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#define SOCKET_PATH "/tmp/tcp_turbo.sock"
#define PID_FILE "/var/run/tcp_turbo.pid"

enum class CommandType {
    ENABLE,
    DISABLE,
    CUSTOM,
    GET_STATS,
    STOP_DAEMON,
    PING
};

struct TcpStats {
    long long in_segs = 0;
    long long out_segs = 0;
    long long retrans_segs = 0;
    long long in_errs = 0;
    long long out_rsts = 0;
    double retrans_rate = 0.0;
};