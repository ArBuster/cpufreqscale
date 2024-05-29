#include "CpuGroup.hpp"
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>

// 构造函数
// group_setting: 配置文件读取到的分组设置
CpuGroup::CpuGroup(std::vector<CpuGroupSetting> &group_setting)
{
    this->group_setting = group_setting;
}

// 枚举cpu逻辑核心数量
// 成功返回true,失败返回false
bool CpuGroup::enumrate_cpu_num()
{
    bool ret_value = false;

    DIR *dp = opendir("/sys/devices/system/cpu/cpufreq/");
    if (dp != nullptr)
    {
        dirent *dir = nullptr;
        while ((dir = readdir(dp)) != nullptr)
        {
            if (strstr(dir->d_name, "policy") != nullptr)
            {
                cpu_num++;
            }
        }
        closedir(dp);

        if (cpu_num > 0)
        {
            ret_value = true;
        }
    }
    else
    {
        printf("打开目录: /sys/devices/system/cpu/cpufreq/ 失败\n");
    }

    return ret_value;
}

// 启动cpu分组调控
void CpuGroup::start()
{
    if (initialize() == false)
    {
        return;
    }

    cpu_stat->start();
    std::vector<float> cpu_usage;
    while (running)
    {
        cpu_stat->get_utilization_rate(cpu_usage);
        for (std::vector<std::shared_ptr<CpuFreq>>::iterator it = groups.begin(); it != groups.end(); it++)
        {
            (*it)->scaling(cpu_usage);
        }
    }
}

// 退出cpu分组调控
void CpuGroup::stop()
{
    running = false;
    cpu_stat->stop();
}

// 初始化
// 成功返回true, 失败返回false
bool CpuGroup::initialize()
{
    bool ret_value = enumrate_cpu_num();

    if (ret_value == true)
    {
        ret_value = set_groups();
    }

    if (ret_value == true)
    {
        print_groups_setting();
        cpu_stat = std::shared_ptr<CpuStat>(new CpuStat(cpu_num));
    }

    return ret_value;
}

// 设置分组
// 依据group_setting中的内容进行设置
// 将group_setting中没设置的cpu核心编号, 每个编号单独设置成一组
// group_setting[0]是全局设置, 是每个分组默认的设置, 不能有cpu编号设置
// 如果group_setting中有的分组设置超出cpu编号范围，需要去除
bool CpuGroup::set_groups()
{
    bool ret_value = true;

    std::vector<CpuGroupSetting> settings;

    // 将group_setting中没设置的cpu核心编号, 每个编号单独设置成一组
    for (unsigned int i = 0; i < cpu_num; i++)
    {
        bool find = false;
        for (std::vector<CpuGroupSetting>::iterator it = group_setting.begin(); it != group_setting.end(); it++)
        {
            if (std::find(it->cpu_number.begin(), it->cpu_number.end(), i) != it->cpu_number.end())
            {
                find = true;
                break;
            }
        }

        if (find == false)
        {
            CpuGroupSetting s;
            s.cpu_number.push_back(i);
            settings.push_back(s);
        }
    }

    // group_setting[0]是全局设置, 是每个分组默认的设置, 不能有cpu编号设置
    if (group_setting.empty() == false && group_setting[0].cpu_number.empty() == true)
    {
        for (std::vector<CpuGroupSetting>::iterator it = settings.begin(); it != settings.end(); it++)
        {
            it->base_frequency = group_setting[0].base_frequency;
            it->min_frequency = group_setting[0].min_frequency;
            it->max_frequency = group_setting[0].max_frequency;
            it->upgrade_usage = group_setting[0].upgrade_usage;
            it->downgrade_usage = group_setting[0].downgrade_usage;
        }
    }

    // 如果group_setting中有的分组设置超出cpu编号范围，需要去除
    for (std::vector<CpuGroupSetting>::iterator it = group_setting.begin(); it != group_setting.end();)
    {
        for (std::vector<unsigned int>::iterator num_it = it->cpu_number.begin(); num_it != it->cpu_number.end();)
        {
            if (*num_it >= cpu_num)
            {
                it->cpu_number.erase(num_it);
                continue;
            }
            num_it++;
        }

        if (it->cpu_number.empty() == true)
        {
            group_setting.erase(it);
            continue;
        }

        settings.push_back(*it);
        it++;
    }

    // 创建CpuFreq对象
    for (std::vector<CpuGroupSetting>::iterator it = settings.begin(); it != settings.end(); it++)
    {
        std::shared_ptr<CpuFreq> cf(new CpuFreq(*it));
        if (cf->initialize() == true)
        {
            groups.push_back(cf);
        }
        else
        {
            ret_value = false;
            break;
        }
    }

    return ret_value;
}

// 打印分组设置
void CpuGroup::print_groups_setting()
{
    CpuGroupSetting gs;
    for (std::vector<std::shared_ptr<CpuFreq>>::iterator it = groups.begin(); it != groups.end(); it++)
    {
        (*it)->get_freq_setting(gs);
        printf("\ncpu number: ");
        for (std::vector<unsigned int>::iterator num = gs.cpu_number.begin(); num != gs.cpu_number.end(); num++)
        {
            printf("%u, ", *num);
        }
        printf("\nbase frequency: %uMHz\n", gs.base_frequency / 1000);
        printf("min frequency: %uMHz\n", gs.min_frequency / 1000);
        printf("max frequency: %uMHz\n", gs.max_frequency / 1000);
        printf("upgrade usage: %.2f\n", gs.upgrade_usage);
        printf("downgrade usage: %.2f\n", gs.downgrade_usage);
    }
    fflush(stdout);
}
