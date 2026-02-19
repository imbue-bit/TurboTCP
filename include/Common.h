#pragma once
#include <string>
#include <vector>

#define SOCKET_PATH "/tmp/tcp_turbo.sock"
#define PID_FILE "/var/run/tcp_turbo.pid"
#define DEFAULT_RELAY_LOCAL_PORT 12000 

struct TcpStats {
    long long in_segs = 0;
    long long out_segs = 0;
    long long retrans_segs = 0;
    double retrans_rate = 0.0;
    long long relay_bytes_tx = 0;
    long long relay_bytes_rx = 0;
    int relay_active_conns = 0;
};

enum class CommandType {
    ENABLE,
    DISABLE,
    CUSTOM,
    GET_STATS,
    STOP_DAEMON,
    PING
};