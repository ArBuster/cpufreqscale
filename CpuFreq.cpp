#include "CpuFreq.hpp"
#include <algorithm>

#define _GRADE_NUM_ 20

// 构造函数
CpuFreq::CpuFreq(CpuGroupSetting &setting)
{
    this->cpu_number = setting.cpu_number;
    this->base_frequency = setting.base_frequency;
    this->min_frequency = setting.min_frequency;
    this->max_frequency = setting.max_frequency;
    this->upgrade_usage = setting.upgrade_usage;
    this->downgrade_usage = setting.downgrade_usage;
}

// 析构函数
CpuFreq::~CpuFreq()
{
    reset_default_setting();
}

// 打开指定的policy文件路径
// number: cpu编号
// file_name: 文件名
// mode: 打开方式
// 成功返回true, 失败返回false
bool CpuFreq::open_policy_path(std::shared_ptr<std::fstream> &ref_out,
                               const unsigned int number, const char *file_name,
                               std::ios_base::openmode mode)
{
    bool ret_value = true;

    std::string path("/sys/devices/system/cpu/cpufreq/policy" + std::to_string(number) + "/" + file_name);
    std::shared_ptr<std::fstream> f(new std::fstream(path, mode));

    if (f->is_open() == true)
    {
        ref_out = f;
    }
    else
    {
        printf("打开文件: %s 失败\n", path.data());
        ret_value = false;
    }

    return ret_value;
}

// 校验并设置参数
// 成功返回true, 失败返回false
bool CpuFreq::verify_paramters()
{
    if (cpu_number.empty() == true)
    {
        return false;
    }

    // 初始化base_frequency, min_frequency. 取所有cpu中的最小值, max_frequency取最大值
    unsigned int base_freq = 0, min_freq = 0, max_freq = 0, temp;
    std::shared_ptr<std::fstream> fp;
    for (std::vector<unsigned int>::iterator it = cpu_number.begin(); it != cpu_number.end(); it++)
    {
        if (open_policy_path(fp, *it, "base_frequency", std::ios::in) == true)
        {
            *fp >> temp;
            base_freq = (base_freq != 0 ?: std::min(base_freq, temp), temp);
        }

        if (open_policy_path(fp, *it, "cpuinfo_min_freq", std::ios::in) == true)
        {
            *fp >> temp;
            min_freq = (min_freq != 0 ?: std::min(min_freq, temp), temp);
        }

        if (open_policy_path(fp, *it, "cpuinfo_max_freq", std::ios::in) == true)
        {
            *fp >> temp;
            max_freq = std::max(max_freq, temp);
        }
    }

    if (base_freq == 0 || min_freq == 0 || max_freq == 0)
    {
        return false;
    }

    if (base_frequency < min_freq || base_frequency > max_freq)
    {
        base_frequency = base_freq;
    }
    if (min_frequency < min_freq || min_frequency > max_freq)
    {
        min_frequency = min_freq;
    }
    if (max_frequency < min_freq || max_frequency > max_freq)
    {
        max_frequency = max_freq;
    }
    if (upgrade_usage <= 0 || upgrade_usage > 100)
    {
        upgrade_usage = 50;
    }
    if (downgrade_usage <= 0 || downgrade_usage > upgrade_usage)
    {
        downgrade_usage = std::min(upgrade_usage, (float)50);
    }

    return true;
}

// 打开本组cpu编号对应的scaling_max_freq文件流
// 成功返回true, 出错返回false
bool CpuFreq::open_scaling_max_freq()
{
    fsm_maxfreq.reserve(cpu_number.size());
    std::shared_ptr<std::fstream> fp;
    for (std::vector<unsigned int>::iterator it = cpu_number.begin(); it != cpu_number.end(); it++)
    {
        if (open_policy_path(fp, *it, "scaling_max_freq", std::ios::out) == true)
        {
            fsm_maxfreq.push_back(fp);
        }
    }

    return true;
}

// 初始化
// 成功返回true, 失败返回false
bool CpuFreq::initialize()
{
    bool ret_value = verify_paramters();
    if (ret_value == true)
    {
        ret_value = open_scaling_max_freq();
    }

    if (ret_value == true)
    {
        ret_value = set_default_setting();
    }

    if (ret_value == true)
    {
        grade_initialize();
    }

    return ret_value;
}

// 初始化默认设置
// 成功返回true, 出错返回false
bool CpuFreq::set_default_setting()
{
    bool ret_value = true;

    for (std::vector<std::shared_ptr<std::fstream>>::iterator fp = fsm_maxfreq.begin(); fp != fsm_maxfreq.end(); fp++)
    {
        (**fp) << base_frequency;
        (*fp)->flush();
    }

    std::shared_ptr<std::fstream> fp;
    for (std::vector<unsigned int>::iterator it = cpu_number.begin(); it != cpu_number.end(); it++)
    {
        if (open_policy_path(fp, *it, "scaling_min_freq", std::ios::out) == true)
        {
            *fp << min_frequency;
        }
        else
        {
            ret_value = false;
            break;
        }

        if (open_policy_path(fp, *it, "scaling_governor", std::ios::in | std::ios::out) == true)
        {
            std::string governor;
            *fp >> governor;
            scaling_governor_bak.push_back(governor);
            if (governor != "powersave")
            {
                fp->seekg(0, std::ios::beg);
                *fp << "powersave";
            }
        }
        else
        {
            ret_value = false;
            break;
        }
    }

    return ret_value;
}

// 恢复成默认设置
// 把scaling_max_freq设置成cpuinfo_max_freq
// 把scaling_min_freq设置成cpuinfo_min_freq
// 成功返回true, 出错返回false
bool CpuFreq::reset_default_setting()
{
    bool ret_value = true;

    std::shared_ptr<std::fstream> fp_1, fp_2;
    for (size_t i = 0; i < cpu_number.size(); i++)
    {
        if (open_policy_path(fp_1, cpu_number[i], "scaling_min_freq", std::ios::out) == true &&
            open_policy_path(fp_2, cpu_number[i], "cpuinfo_min_freq", std::ios::in) == true)
        {
            *fp_1 << fp_2->rdbuf();
        }
        else
        {
            ret_value = false;
        }

        if (open_policy_path(fp_1, cpu_number[i], "scaling_max_freq", std::ios::out) == true &&
            open_policy_path(fp_2, cpu_number[i], "cpuinfo_max_freq", std::ios::in) == true)
        {
            *fp_1 << fp_2->rdbuf();
        }
        else
        {
            ret_value = false;
        }

        if (open_policy_path(fp_1, cpu_number[i], "scaling_governor", std::ios::in | std::ios::out) == true)
        {
            std::string governor;
            *fp_1 >> governor;
            if (governor != scaling_governor_bak[i])
            {
                fp_1->seekg(0, std::ios::beg);
                *fp_1 << scaling_governor_bak[i];
            }
        }
        else
        {
            ret_value = false;
        }
    }

    return ret_value;
}

// 获取本组cpu的设置
// ref_out: 接收设置的引用
void CpuFreq::get_freq_setting(CpuGroupSetting &ref_out)
{
    ref_out.cpu_number = cpu_number;
    ref_out.base_frequency = base_frequency;
    ref_out.min_frequency = min_frequency;
    ref_out.max_frequency = max_frequency;
    ref_out.upgrade_usage = upgrade_usage;
    ref_out.downgrade_usage = downgrade_usage;
}

// 初始化档位设置
void CpuFreq::grade_initialize()
{
    unsigned int freq_step = std::max((max_frequency - base_frequency) / _GRADE_NUM_, (unsigned int)50000);
    for (unsigned int freq = base_frequency; freq < max_frequency; freq += freq_step)
    {
        frequency_grade.push_back(freq);
    }
    frequency_grade.push_back(max_frequency);
    grade_size = frequency_grade.size() - 1;

    if (grade_size > 0)
    {
        float usage_step = (100 - upgrade_usage) / grade_size;
        for (float usage = upgrade_usage; usage < 100; usage += usage_step)
        {
            up_usage_grade.push_back(usage);
        }
        up_usage_grade.push_back(100);

        usage_step = downgrade_usage / grade_size;
        for (float usage = downgrade_usage; usage > 0; usage -= usage_step)
        {
            down_usage_grade.push_back(usage);
        }
        down_usage_grade.push_back(0);
    }
}

// 调整cpu频率上限
// cpu_usage: 最近一次读取的cpu使用率数据, 下标就是cpu编号
// 调节算法:
// 如果上次数据才调整过频率上限, 忽略本次读取到的使用率数据
// 将base_frequency和max_frequency之间的差值划分成10个档位, 最小调整值不低于50MHz
// 将0-downgrade_usage和upgrade_usage到100之间的差值划分成10个档位,
// 当前使用率落在哪个档位, 就向上或者向下调整频率对应的档位
void CpuFreq::scaling(std::vector<float> &cpu_usage)
{
    if (grade_size <= 0)
    {
        return;
    }

    // 选取本组cpu中使用率最高的那一个核心
    float usage = 0;
    for (std::vector<unsigned int>::iterator it = cpu_number.begin(); it != cpu_number.end(); it++)
    {
        if (cpu_usage[*it] > usage)
        {
            usage = cpu_usage[*it];
        }
    }

    // 上调档位为正整数，下调档位为负整数
    int grade = 0;
    if (scaled != 1 && usage > upgrade_usage)
    {
        std::vector<float>::iterator it = std::lower_bound(up_usage_grade.begin(), up_usage_grade.end(), usage);
        grade = std::distance(up_usage_grade.begin(), it);
    }
    else if (scaled != -1 && usage < downgrade_usage)
    {
        std::vector<float>::iterator it = std::upper_bound(down_usage_grade.begin(), down_usage_grade.end(), usage, std::greater_equal<float>());
        grade = -std::distance(down_usage_grade.begin(), it);
    }
    else if (scaled != 0)
    {
        scaled = 0;
    }

    set_max_frequency(grade);
}

// 调整最大频率
// grade: 要设置的档位
void CpuFreq::set_max_frequency(const int grade)
{
    if (grade > 0 && current_frequency_grade < grade_size)
    {
        current_frequency_grade = std::min(current_frequency_grade + grade, grade_size);
        scaled = 1;
    }
    else if (grade < 0 && current_frequency_grade > 0)
    {
        current_frequency_grade = std::max(current_frequency_grade + grade, 0);
        scaled = -1;
    }

    if (scaled != 0)
    {
        for (std::vector<std::shared_ptr<std::fstream>>::iterator fp = fsm_maxfreq.begin(); fp != fsm_maxfreq.end(); fp++)
        {
            (**fp) << frequency_grade[current_frequency_grade];
            (*fp)->flush();
        }
    }
}
