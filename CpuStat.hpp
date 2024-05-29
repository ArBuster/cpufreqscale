#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// 读取cpu使用率的类
class CpuStat
{
private:
    // cpu每个逻辑核心的最近一次读取到的使用率
    std::vector<float> cpu_utilization_rate;
    // cpu逻辑核心数量
    unsigned int cpu_num;
    // 监控线程
    std::thread monitor_thread;
    // 读写std::vector<float> cpu_utilization_rate的锁
    std::mutex mtx;
    // 读写std::vector<float> cpu_utilization_rate的信号量
    std::condition_variable cond_var;
    // 读写线程同步的标志
    bool produced;
    // 监控线程循环判断条件
    std::atomic_bool running;

    void monitor();
    void convert_stat_string(uint64_t stat[2], const std::string &line);
    void caculate_utilization(const uint64_t new_stat[][2], const uint64_t old_stat[][2]);

public:
    CpuStat(const unsigned int cpu_num);
    ~CpuStat();
    bool start();
    bool stop();
    void get_utilization_rate(std::vector<float> &ref_out);
};
