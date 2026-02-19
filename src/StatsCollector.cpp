#include "StatsCollector.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

TcpStats StatsCollector::collect() {
    TcpStats stats;
    std::ifstream file("/proc/net/snmp");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 4) == "Tcp:") {
            // 这里我们偷一下懒，直接读取第二次出现的 "Tcp:" 行，那是数据行（补药骂我🥺）
            // 第一次出现的是 Header
            std::string line2;
            if (std::getline(file, line2) && line2.substr(0, 4) == "Tcp:") {
                std::stringstream ss(line2);
                std::string tmp;
                ss >> tmp; // "Tcp:"
                
                // 跳过不需要的字段 (根据 Linux Kernel 文档)
                // RtoAlgorithm, RtoMin, RtoMax, MaxConn, ActiveOpens, PassiveOpens, AttemptFails, EstabResets, CurrEstab
                for(int i=0; i<9; ++i) ss >> tmp;
                
                ss >> stats.in_segs;       // 10: InSegs
                ss >> stats.out_segs;      // 11: OutSegs
                ss >> stats.retrans_segs;  // 12: RetransSegs
                ss >> stats.in_errs;       // 13: InErrs
                ss >> stats.out_rsts;      // 14: OutRsts
                
                if (stats.out_segs > 0) {
                    stats.retrans_rate = (double)stats.retrans_segs / (double)stats.out_segs * 100.0;
                }
                break;
            }
        }
    }
    return stats;
}