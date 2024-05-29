#pragma once

#include "CpuGroupSetting.h"
#include "CpuFreq.hpp"
#include "CpuStat.hpp"
#include <vector>
#include <memory>

// 记录cpu分组的类
class CpuGroup
{
private:
    // cpu逻辑核心数量
    unsigned int cpu_num = 0;
    // 循环退出标识
    bool running = true;
    // 分组设置
    std::vector<CpuGroupSetting> group_setting;
    // cpu分组对象的数组
    std::vector<std::shared_ptr<CpuFreq>> groups;
    // CpuStat对象指针, 读取使用率
    std::shared_ptr<CpuStat> cpu_stat;

    bool enumrate_cpu_num();
    bool initialize();
    bool set_groups();
    void print_groups_setting();

public:
    CpuGroup(std::vector<CpuGroupSetting> &group_setting);
    void start();
    void stop();
};
