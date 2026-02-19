#pragma once
#include <string>
#include <vector>

namespace TurboTCP {
    enum class CommandType {
        ENABLE,
        DISABLE,
        CUSTOM,
        GET_STATS,
        REGISTER_SERVICE,
        STOP_DAEMON,
        UNKNOWN
    };

    struct Request {
        CommandType type;
        std::string payload;
    };

    struct TcpStats {
        long long in_segs = 0;
        long long out_segs = 0;
        long long retrans_segs = 0;
        long long curr_estab = 0;
        double rtt_smooth = 0.0;
    };

    struct Response {
        bool success;
        std::string message;
        TcpStats stats;
    };

    static const char* SOCKET_PATH = "/tmp/turbotcp.sock";
    static const char* PID_FILE = "/tmp/turbotcp.pid";
}