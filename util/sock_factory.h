/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: sock_ops.h
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 11-29-2022 10:02:08
 * @version $Revision$
 *
 **************************************************************************/

#pragma once

#include "common.h"


class SockFactory {
public:
    SockFactory() = delete;

    static const int kBeginPort = 10001;

    static sockaddr* p_remote_addr_;
    static sockaddr_in remote_addr_;

    static std::vector<int> MakeSockets(int jobs_num) {
        std::vector<int> fds;
        sockaddr_in local_addr;
        local_addr.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &local_addr.sin_addr);

        for (auto i = kBeginPort; i < kBeginPort+jobs_num; ++i) {
            fds.emplace_back(socket(AF_INET, SOCK_STREAM, 0));
            local_addr.sin_port = htons(i);
            bind(fds.back(), reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr));
        }

        return fds;
    }

    static sockaddr* GetRemoteAddr(const std::string& ip, const int port) {
        if (p_remote_addr_ == nullptr) {
            bzero(&remote_addr_, sizeof(remote_addr_));
            remote_addr_.sin_family = AF_INET;
            inet_pton(AF_INET, ip.c_str(), &remote_addr_.sin_addr);
            remote_addr_.sin_port = htons(port);

            p_remote_addr_ = reinterpret_cast<sockaddr*>(&remote_addr_);
        }

        return p_remote_addr_;
    }

};

sockaddr* SockFactory::p_remote_addr_ = nullptr;
sockaddr_in SockFactory::remote_addr_ {};

