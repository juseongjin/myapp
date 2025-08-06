// ~/lg/LiteRT_app/monitor/monitor.h
#ifndef MONITOR_H_
#define MONITOR_H_

#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

#include "app.h"
namespace monitor {

    inline void ReadCPUStat(unsigned long long& idle, unsigned long long& total) {
        std::ifstream stat_file("/proc/stat");
        std::string line;
        std::getline(stat_file, line);
        std::istringstream iss(line);

        std::string cpu_label;
        unsigned long long user, nice, system, idle_time, iowait, irq, softirq;

        iss >> cpu_label >> user >> nice >> system >> idle_time >> iowait >> irq >> softirq;
        idle = idle_time + iowait;
        total = user + nice + system + idle + irq + softirq;
    }

    inline float GetCPUUtilization() {
        unsigned long long idle1, total1, idle2, total2;
        ReadCPUStat(idle1, total1);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        ReadCPUStat(idle2, total2);

        unsigned long long delta_total = total2 - total1;
        unsigned long long delta_idle = idle2 - idle1;

        if (delta_total == 0) return 0.0f;
        return 100.0f * (1.0f - static_cast<float>(delta_idle) / delta_total);
    }

    inline float GetGPUUtilization() {
        std::ifstream file("/sys/class/kgsl/kgsl-3d0/gpubusy");
        if (!file.is_open()) {
            LOGI("open gpubusy failed: %s", strerror(errno));
            return -1.0f;
        }

        unsigned long long busy = 0, total = 0;
        file >> busy >> total;
        if (total == 0) return 0.0f;
        return 100.0f * static_cast<float>(busy) / total;
    }

    inline void GetUtilization(std::vector<float>& util) {
        util[0] = GetCPUUtilization();
        util[1] = GetGPUUtilization();
        return;
    }

}  // namespace monitor

#endif  // MONITOR_H_
