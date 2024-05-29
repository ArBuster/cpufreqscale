#pragma once

#include "CpuGroupSetting.h"

extern "C" bool parser_args(int argc, char *argv[], std::vector<CpuGroupSetting> &ref_out);
