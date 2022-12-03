#pragma once

#include <spdlog/spdlog.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#include <memory>


//static auto LOG = spdlog::get("test1");
//extern std::shared_ptr<spdlog::logger> LOG;

void func2() {
    spdlog::info("hello ctor2.h");
}
