#pragma once

#include <vector>

// cpu分组设置
struct CpuGroupSetting
{
public:
    // 本组内的cpu编号
    // 必须设置
    std::vector<unsigned int> cpu_number;
    // 本组cpu共同的基准频率（默频）,单位Hz
    // 如未设置, 默认使用base_frequency的值
    unsigned int base_frequency = 0;
    // 本组cpu共同的最低频率,单位Hz
    // 如未设置, 默认使用cpuinfo_min_freq的值
    unsigned int min_frequency = 0;
    // 本组cpu共同的最高频率,单位Hz
    // 如未设置, 默认使用cpuinfo_max_freq的值
    unsigned int max_frequency = 0;
    // 提升频率的使用率分界值, 以组内使用率最高的核心为准, 数值范围0-100
    // 如未设置, 默认值50
    float upgrade_usage = 0;
    // 降低频率的使用率分界值, 以组内使用率最高的核心为准, 数值范围0-100
    // 如未设置, 默认值50
    float downgrade_usage = 0;
};
