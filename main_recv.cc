/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: main_recv.cc
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 12-01-2022 20:47:52
 * @version $Revision$
 *
 **************************************************************************/


#include "recv_flow.h"


DEFINE_int32(jobs, 1, "Specifies the number of jobs (threads) to run simultaneously.");
DEFINE_int32(port, 10049, "Specifies the remote hosts' port.");
DEFINE_string(file_path, "data.recv.arrow", "Specifies  the file path of the arrow file.");
DEFINE_string(remote, "127.0.0.1", "Specifies the remote(Sender) hosts' IP.");

int main(int argc, char**argv) {
    gflags::SetVersionString("0.1.0");
    gflags::SetUsageMessage("Transport Big Arrow File.");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    spdlog::set_pattern("[recv %t] %+ ");
    spdlog::set_level(spdlog::level::info);



    SPDLOG_INFO("hello world!");

    return 0;
}
