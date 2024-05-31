#include "CpuGroupSetting.h"
#include "CpuGroup.hpp"
#include "ConfigParser.h"
#include <signal.h>
#include <memory>
#include <vector>
#include <stdio.h>

std::unique_ptr<CpuGroup> pcg;

// 接收处理SIGTERM信号 (kill 15, systemctrl stop默认发送的信号)
// 用于正常退出进程
void handle_signal_term(int signal_num)
{
    pcg->stop();
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, handle_signal_term);

    std::vector<CpuGroupSetting> cgs;

    if (parser_args(argc, argv, cgs) == true)
    {
        pcg = std::unique_ptr<CpuGroup>(new CpuGroup(cgs));
        pcg->start();
    }
    else
    {
        printf("\nparser json config failed.\n");
    }

    printf("\ncpufreqscale exited.\n");

    return 0;
}
