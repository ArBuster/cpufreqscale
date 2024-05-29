#pragma once

#include "CpuGroupSetting.h"
#include <vector>
#include <fstream>
#include <string>
#include <memory>

// 读写/sys/devices/system/cpu/cpufreq/policy*/*目录的类
class CpuFreq : private CpuGroupSetting
{
private:
    // 本组cpu编号对应的scaling_max_freq文件流
    std::vector<std::shared_ptr<std::fstream>> fsm_maxfreq;
    // 用于标识刚刚调整过频率上限,0没有调整,1上调过,-1下调过
    int scaled = 0;
    // 当前频率上限的档位, 0-10中的任意值
    int current_frequency_grade = 0;
    // 频率划分档位，从base_frequency到max_frequency划分出10档,单位Hz,最小间隔50000Hz
    std::vector<unsigned int> frequency_grade;
    // frequency_grade的大小-1
    int grade_size = 0;
    // 上调频率的使用率档位，从upgrade_usage到100划分出10档
    std::vector<float> up_usage_grade;
    // 下调频率使用率档位，从0到downgrade_usage划分出10档
    std::vector<float> down_usage_grade;
    // scaling_governor的备份
    std::vector<std::string> scaling_governor_bak;

    bool open_policy_path(std::shared_ptr<std::fstream> &ref_out,
                          const unsigned int number, const char *file_name,
                          std::ios_base::openmode mode);
    bool verify_paramters();
    bool open_scaling_max_freq();
    bool set_default_setting();
    bool reset_default_setting();
    void grade_initialize();
    void set_max_frequency(const int grade);

public:
    CpuFreq(CpuGroupSetting &setting);
    ~CpuFreq();
    bool initialize();
    void get_freq_setting(CpuGroupSetting &ref_out);
    void scaling(std::vector<float> &cpu_usage);
};
