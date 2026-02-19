#include "SystemTuner.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>

const std::map<std::string, std::string> kOptimizedParams = {
    {"net.core.default_qdisc", "fq"},              // BBR 前置要求
    {"net.ipv4.tcp_congestion_control", "bbr"},    // Google BBR 拥塞控制
    {"net.ipv4.tcp_window_scaling", "1"},          // 启用窗口缩放
    {"net.core.rmem_max", "16777216"},             // 接收缓冲区最大值 16MB
    {"net.core.wmem_max", "16777216"},             // 发送缓冲区最大值 16MB
    {"net.ipv4.tcp_rmem", "4096 87380 16777216"},  // 自动调整 TCP 接收缓冲
    {"net.ipv4.tcp_wmem", "4096 65536 16777216"},  // 自动调整 TCP 发送缓冲
    {"net.ipv4.tcp_slow_start_after_idle", "0"},   // 关闭空闲后的慢启动
    {"net.ipv4.tcp_mtu_probing", "1"},             // 开启 MTU 探测，防止黑洞
    {"net.ipv4.tcp_fastopen", "3"},                // 开启 TCP Fast Open (Client+Server)
    {"net.ipv4.tcp_syncookies", "1"},              // 防 SYN Flood
    {"net.ipv4.tcp_tw_reuse", "1"},                // 允许重用 TIME_WAIT socket
    {"net.ipv4.ip_local_port_range", "1024 65535"},// 扩大本地端口范围
    {"net.core.somaxconn", "65535"}                // 增加监听队列上限
};

void SystemTuner::setSysctl(const std::string& key, const std::string& value) {
    std::string path = "/proc/sys/" + key;
    size_t pos = 0;
    while((pos = path.find('.', pos)) != std::string::npos) {
        path.replace(pos, 1, "/");
        pos++;
    }
    if (original_values_.find(key) == original_values_.end()) {
        original_values_[key] = getSysctl(key);
    }
    std::ofstream ofs(path);
    if (ofs.is_open()) {
        ofs << value;
    } else {
        std::cerr << "Error writing to " << path << std::endl;
    }
}
std::string SystemTuner::getSysctl(const std::string& key) {
    std::string path = "/proc/sys/" + key;
    size_t pos = 0;
    while((pos = path.find('.', pos)) != std::string::npos) {
        path.replace(pos, 1, "/");
        pos++;
    }
    std::ifstream ifs(path);
    std::string val;
    if (ifs >> val) return val;
    return "";
}
void SystemTuner::enableOptimization() {
    if (is_optimized_) return;
    std::cout << "Applying extreme optimizations..." << std::endl;
    for (const auto& [key, val] : kOptimizedParams) {
        setSysctl(key, val);
    }
    is_optimized_ = true;
}

void SystemTuner::disableOptimization() {
    if (!is_optimized_) return;
    std::cout << "Reverting optimizations..." << std::endl;
    for (const auto& [key, val] : original_values_) {
        setSysctl(key, val);
    }
    is_optimized_ = false;
}
void SystemTuner::applyCustom(const std::map<std::string, std::string>& params) {
    std::cout << "Applying custom optimizations..." << std::endl;
    for (const auto& [key, val] : params) {
        setSysctl(key, val);
    }
}