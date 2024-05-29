#include "CpuStat.hpp"
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <chrono>
#include <string.h>

// 构造函数
// cpu_num：cpu逻辑核心数量
CpuStat::CpuStat(const unsigned int cpu_num)
{
    this->cpu_num = cpu_num;
    this->running = true;
    this->cpu_utilization_rate.resize(cpu_num);
    this->produced = false;
}

// 析构函数
CpuStat::~CpuStat()
{
    stop();
}

// 开始监控
bool CpuStat::start()
{
    monitor_thread = std::thread(&CpuStat::monitor, this);
    return true;
}

// 停止监控
bool CpuStat::stop()
{
    if (running == true && monitor_thread.joinable() == true)
    {
        running = false;
        cond_var.notify_all();
        monitor_thread.join();
    }
    return true;
}

// 监控cpu使用率,定期读取/proc/stat并计算核心使用率
void CpuStat::monitor()
{
    uint64_t stat[2][cpu_num][2];

    std::string line;
    // 保留512字节的空间作为读取缓存
    line.reserve(512);

    std::chrono::steady_clock::time_point time_start;
    long long time_escape = 0;
    uint8_t k = 0;

    std::ifstream stat_file("/proc/stat", std::ios::in);
    if(stat_file.is_open() == false)
    {
        printf("打开文件: /proc/stat 失败\n");
        fflush(stdout);
        return;
    }

    while (running == true)
    {
        stat_file.sync();
        time_start = std::chrono::steady_clock::now();

        memset(stat[k], 0, sizeof(uint64_t) * cpu_num * 2);
        stat_file.seekg(0, std::ios::beg);
        std::getline(stat_file, line);
        for (unsigned int i = 0; i < cpu_num; i++)
        {
            std::getline(stat_file, line);
            convert_stat_string(stat[k][i], line);
        }

        caculate_utilization(stat[k], stat[k ^ 1]);

        k ^= 1;
        time_escape = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - time_start).count();

        // 休眠100ms
        if (time_escape < 100000)
        {
            usleep(100000 - time_escape);
        }
    }
}

// 将读取到的/proc/stat文本数据转换成总cpu时间和空闲时间
// line：读取到的/proc/stat文本数据
// stat：stat[0] cpu总使用时间, stat[1] cpu空闲时间
void CpuStat::convert_stat_string(uint64_t stat[2], const std::string &line)
{
    std::istringstream ss(line);
    std::string temp;
    temp.reserve(128);

    std::getline(ss, temp, ' ');
    for (unsigned int i = 0; i < 7; i++)
    {
        std::getline(ss, temp, ' ');
        stat[0] += std::stoull(temp);
        if (i == 3)
        {
            stat[1] = std::stoull(temp);
        }
    }
}

// 计算cpu使用率
// 算法: 用最新一次的stat数据 - 上一次数据, 1 - (idle time / total time)就是使用率
// new_stat, old_stat: 两次读取到的cpu每个逻辑核心的使用率, stat[0] cpu总使用时间, stat[1] cpu空闲时间
void CpuStat::caculate_utilization(const uint64_t new_stat[][2], const uint64_t old_stat[][2])
{
    std::unique_lock<std::mutex> lock(mtx);
    for (unsigned int i = 0; i < cpu_num; i++)
    {
        cpu_utilization_rate[i] = 100 - (float)((new_stat[i][1] - old_stat[i][1]) * 100) / (new_stat[i][0] - old_stat[i][0]);
    }
    produced = true;
    cond_var.notify_one();
}

// 获取cpu使用率
// ref_out: 接收核心使用率的引用
void CpuStat::get_utilization_rate(std::vector<float> &ref_out)
{
    std::unique_lock<std::mutex> lock(mtx);
    if (produced == false)
    {
        cond_var.wait(lock);
    }
    ref_out = cpu_utilization_rate;
    produced = false;
}
