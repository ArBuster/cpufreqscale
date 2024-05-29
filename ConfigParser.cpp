#include "ConfigParser.h"
#include <unistd.h>
#include <stdio.h>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include <fstream>
#include <string.h>
#include <regex>
#include <string>
#include <algorithm>

extern char *optarg;

// 打印使用帮助
void print_usage()
{
    printf("cpufreqscale -c config_path\n");
}

// 解析分组的cpu编号
// number: cpu编号数组，数组元素可以是uint32_t或者string
// ref_out: 接收cpu分组编号的数组
// 成功解析返回true, 没有解析出编号返回false
bool parser_group_cpu_number(rapidjson::Value &number, std::vector<unsigned int> &ref_out)
{
    for (rapidjson::Value &v : number.GetArray())
    {
        if (v.IsUint() == true)
        {
            ref_out.push_back(v.GetUint());
        }
        else if (v.IsString() == true)
        {
            std::string item(v.GetString());
            std::smatch matches;
            if (std::regex_match(item, matches, std::regex(R"((\d+)-(\d+))")) == true)
            {
                for (unsigned int i = std::stoul(matches[1]); i <= std::stoul(matches[2]); i++)
                {
                    ref_out.push_back(i);
                }
            }
            else if (std::regex_match(item, std::regex(R"(\d+)")) == true)
            {
                ref_out.push_back(std::stoul(item));
            }
        }
    }

    if (ref_out.empty() == true)
    {
        return false;
    }

    return true;
}

// 解析json对象中的特定字段
// value: json dom对象引用
// cgs: CpuGroupSetting对象引用
bool parser_object_settings(rapidjson::Value &value, CpuGroupSetting &cgs)
{
    for (rapidjson::Value::MemberIterator it = value.MemberBegin(); it != value.MemberEnd(); it++)
    {
        if (strcmp(it->name.GetString(), "base") == 0 && it->value.IsUint() == true)
        {
            cgs.base_frequency = it->value.GetUint() * 1000;
        }
        else if (strcmp(it->name.GetString(), "min") == 0 && it->value.IsUint() == true)
        {
            cgs.min_frequency = it->value.GetUint() * 1000;
        }
        else if (strcmp(it->name.GetString(), "max") == 0 && it->value.IsUint() == true)
        {
            cgs.max_frequency = it->value.GetUint() * 1000;
        }
        else if (strcmp(it->name.GetString(), "up") == 0 && it->value.IsNumber() == true)
        {
            float temp = it->value.GetFloat();
            if (temp > 0 && temp <= 100)
            {
                cgs.upgrade_usage = temp;
            }
        }
        else if (strcmp(it->name.GetString(), "down") == 0 && it->value.IsNumber() == true)
        {
            float temp = it->value.GetFloat();
            if (temp > 0 && temp <= 100)
            {
                cgs.downgrade_usage = temp;
            }
        }
        else if (strcmp(it->name.GetString(), "number") == 0 && it->value.IsArray() == true)
        {
            if (parser_group_cpu_number(it->value, cgs.cpu_number) == false)
            {
                return false;
            }
        }
    }

    return true;
}

// 检查cpu分组编号是否存在重复项目
// 存在重复项目返回true, 没有重复返回false
bool test_duplicate_cpu_number(const std::vector<CpuGroupSetting> &cgs_vec)
{
    bool ret_value = false;

    std::vector<unsigned int> cpu_number;
    for (std::vector<CpuGroupSetting>::const_iterator it = cgs_vec.cbegin(); it != cgs_vec.cend(); it++)
    {
        if (it->cpu_number.empty() == false)
        {
            cpu_number.insert(cpu_number.end(), it->cpu_number.cbegin(), it->cpu_number.cend());
        }
    }

    std::sort(cpu_number.begin(), cpu_number.end());
    for (std::vector<unsigned int>::iterator it = std::adjacent_find(cpu_number.begin(), cpu_number.end()); it != cpu_number.end(); it = std::adjacent_find(it + 1, cpu_number.end()))
    {
        printf("存在重复的cpu分组编号: %u\n", *it);
        ret_value = true;
    }

    return ret_value;
}

// 解析配置文件
// file_path: 配置文件的路径
// ref_out: 解析后的CpuGroupSetting的数组
// 成功返回true, 失败返回false
bool parser_config_file(const char *file_path, std::vector<CpuGroupSetting> &ref_out)
{
    std::ifstream ifs(file_path, std::ios::in);
    if (ifs.is_open() == false)
    {
        printf("打开文件: %s 失败\n", file_path);
        return false;
    }

    ref_out.clear();
    ref_out.push_back(CpuGroupSetting());

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream<rapidjson::kParseCommentsFlag>(isw);

    // 解析global项
    if (doc.HasMember("global") == true && doc["global"].IsObject() == true && doc["global"].HasMember("number") == false)
    {
        parser_object_settings(doc["global"], ref_out[0]);
    }

    // 解析groups项
    if (doc.HasMember("groups") == true && doc["groups"].IsArray() == true)
    {
        for (rapidjson::Value &v : doc["groups"].GetArray())
        {
            if (v.IsObject() == true && v.HasMember("number") == true)
            {
                CpuGroupSetting cgs;
                if (parser_object_settings(v, cgs) == true)
                {
                    ref_out.push_back(cgs);
                }
            }
        }
    }

    return !test_duplicate_cpu_number(ref_out);
}

// 解析命令行参数
// ref_out: 解析后的CpuGroupSetting的数组
// 成功返回true, 失败返回false
bool parser_args(int argc, char *argv[], std::vector<CpuGroupSetting> &ref_out)
{
    bool ret_value = false;

    for (int p; EOF != (p = getopt(argc, argv, "hc:"));)
    {
        if (p == 'c')
        {
            ret_value = parser_config_file(optarg, ref_out);
            break;
        }
        else if (p == '?' || p == 'h')
        {
            print_usage();
            break;
        }
    }

    return ret_value;
}
