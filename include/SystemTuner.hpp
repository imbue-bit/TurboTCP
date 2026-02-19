#pragma once
#include <map>
#include <string>

class SystemTuner {
public:
    void enableOptimization();
    void disableOptimization();
    void applyCustom(const std::map<std::string, std::string>& params);
    
private:
    void setSysctl(const std::string& key, const std::string& value);
    std::string getSysctl(const std::string& key);
    std::map<std::string, std::string> original_values_;
    bool is_optimized_ = false;
};