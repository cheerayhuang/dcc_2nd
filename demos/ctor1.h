#pragma once

#include <spdlog/spdlog.h>
#include "spdlog/sinks/stdout_color_sinks.h"



void func1() {
    auto LOG = spdlog::get("test1");
    LOG->info("hello ctor1.h");
}
