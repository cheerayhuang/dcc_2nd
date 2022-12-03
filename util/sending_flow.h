/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: sending_flow.h
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 11-30-2022 19:46:22
 * @version $Revision$
 *
 **************************************************************************/



#pragma once

#include "common.h"

#include "file_slice.h"
#include "zstd_compress.h"
#include "sock_factory.h"
#include "recv_flow.h"

DECLARE_string(remote);
DECLARE_int32(port);

class SendingFlow {
public:
    SendingFlow() = delete;

    static const size_t kMaxTCPSendBuff = 4 * 1024 * 1024;

    static int Init(size_t file_size, unsigned short slices_num) {
        if (auto r = ZSTDCompressUtil::Init(); r != 0) {
            return -1;
        }


        if (auto r = SendFileSizeOnly(file_size, slices_num); r != 0) {
            return -1;
        }

        return 0;
    };

    static int Destory() {
        ZSTDCompressUtil::Destory();

        return 0;
    };

    // thread function
    static void Run(std::shared_ptr<FileSlice> slice) {
        if (auto r = ZSTDCompressUtil::Compress(slice); !r) {
            SPDLOG_ERROR("ZSTD compress failed. File slice info: index={}, len={}.", slice->index(), slice->len());
            return ;
        }

        if (auto r = SendFileSlice(slice); r != 0) {
            return ;
        }
    }

    static int SendFileSlice(std::shared_ptr<FileSlice> slice) {
        auto &chunks = slice->compress_buf();

        auto remote_addr = SockFactory::GetRemoteAddr(FLAGS_remote, FLAGS_port);
        auto fd = socket(AF_INET, SOCK_STREAM, 0);

        if (auto r = connect(fd, remote_addr, sizeof(*remote_addr)); r == -1) {
            SPDLOG_ERROR("Connect to server to send file slice failed.");
            close(fd);
            return -1;
        }

        size_t sum_sent = 0;
        for (auto && c : chunks) {
            if (c.len_ < kMaxTCPSendBuff) {
                if (write(fd, c.c_, c.len_) == -1) {
                    SPDLOG_ERROR("Send file slice at once failed: {} bytes.", c.len_);
                    close(fd);
                    return -1;
                }
                sum_sent += c.len_;
                continue;
            }

            size_t index = 0;
            do {
                auto act_wr_size = (index + kMaxTCPSendBuff > c.len_) ? (c.len_ - index) : kMaxTCPSendBuff;
                if (write(fd, c.c_+index, act_wr_size) == -1) {
                    SPDLOG_ERROR("Send file slice circularly failed: {} bytes.");
                    close(fd);
                    return -1;
                }
                index += act_wr_size;
                sum_sent += act_wr_size;
            } while(index < c.len_);
        }

        if (sum_sent != slice->c_len()) {
            SPDLOG_ERROR("Send slice failed: compress len {} != {} bytes sent.", slice->c_len(), sum_sent);
            close(fd);
            return -1;
        }
        SPDLOG_INFO("Send slice finished. Info: index {}, len {}, c_len {}, sum sent {}.", slice->index(), slice->len(), slice->c_len(), sum_sent);

        close(fd);
        return 0;
    }

    static int SendFileSizeOnly(size_t file_size, unsigned short slices_num) {
        auto remote_addr = SockFactory::GetRemoteAddr(FLAGS_remote, FLAGS_port);
        auto fd = socket(AF_INET, SOCK_STREAM, 0);

        if (auto r = connect(fd, remote_addr, sizeof(*remote_addr)); r == -1) {
            SPDLOG_ERROR("Connect to server to send file size failed.");
            close(fd);
            return -1;
        }

        std::ostringstream header_file_size;
        header_file_size << "S" << file_size << "#" << slices_num;
        if (write(fd, header_file_size.str().c_str(), header_file_size.str().size()) == -1) {
            SPDLOG_ERROR("Send file size header failed.");
            close(fd);
            return -1;
        }

        close(fd);

        return 0;
    }

    using time_point = std::chrono::steady_clock::time_point;
    static int Supervise(time_point start) {
        auto [listen_fd, epoll_fd] = Server::BindSocket(10050);

        epoll_event events[2];

        for (;;) {
            auto num_fds = epoll_wait(epoll_fd, events, 2, -1);
            if (num_fds == -1) {
                SPDLOG_ERROR("Supervise: Epoll wait failed.");
                close(listen_fd);
                close(epoll_fd);
                return -1;
            }
            SPDLOG_INFO("{} fds are active.", num_fds);

            const int kSupvervisorInfoBuffSize = 512;
            char *read_buf = static_cast<char*>(malloc(kSupvervisorInfoBuffSize));

            for (auto i = 0; i < num_fds; ++i) {
                //SPDLOG_INFO("active fd {} == listen_fd {}.", events[i].data.fd, listen_fd);

                if (events[i].data.fd == listen_fd && (events[i].events & EPOLLIN)) {
                    // non-blocking accept
                    while(auto conn_fd = accept(listen_fd, nullptr, nullptr) > 0) {
                        epoll_event ev;
                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = conn_fd;

                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
                            SPDLOG_ERROR("Calling epoll_ctl failed: add conn fd.");
                            close(conn_fd);
                            close(listen_fd);
                            close(epoll_fd);
                            return  -1;
                        }
                    }
                } else if (events[i].events & EPOLLIN) {
                    auto conn_fd = events[i].data.fd;

                    ssize_t num_read = 0;
                    ssize_t read_index =0;

                    do {
                        num_read = read(conn_fd, read_buf + read_index, kSupvervisorInfoBuffSize - read_index);
                        if (Server::IsSocketError(num_read)) {
                            SPDLOG_ERROR("read HEADER(file size) failed. perror():");
                            perror("\t read() failed");
                            free(read_buf);
                            close(conn_fd);
                            close(listen_fd);
                            close(epoll_fd);
                            return -1;
                        }
                        if (num_read == -1 && errno == EINTR) {
                            continue;
                        }

                        // EAGAIN or EWOULDBLOCK
                        if (num_read == -1) {
                            break;
                        }

                        read_index += num_read;
                    } while(kSupvervisorInfoBuffSize - read_index > 0 && num_read != 0);
                    num_read = read_index;
                    if (num_read == 0) {
                        free(read_buf);
                        close(conn_fd);
                        close(listen_fd);
                        close(epoll_fd);
                        return -1;
                    }

                    std::string_view res{read_buf, static_cast<size_t>(num_read)};

                    if (res.find("END.\n") != std::string::npos) {
                        time_point end = std::chrono::steady_clock::now();
                        SPDLOG_WARN("[duration:{}]", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000);
                        res = res.substr(5);
                    }
                    SPDLOG_WARN("[validString:{}]", res);
                }
            }
        }

        return 0;
    }
};
