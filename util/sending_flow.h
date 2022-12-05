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
    static const int kEachSliceHeaderLen = 10;

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

        return ;
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

        Server::SetNonBlocking(fd);

        size_t sum_sent = 0;
        for (auto && c : chunks) {
            auto size = c.len_;
            auto w_index = 0;
            do {
                auto num_write = write(fd, c.c_+w_index, size-w_index);
                if (Server::IsSocketError(num_write)) {
                    SPDLOG_ERROR("Send file slice failed.");
                    perror("write() failed");
                    close(fd);
                    return -1;
                }

                if (num_write == 0) {
                    SPDLOG_INFO("Send file slice: Already sent {} bytes, len {}, the fd is exhausted.", w_index, size);
                    break;
                }

                /*
                if (num_write == -1 && errno == EINTR) {
                    continue;
                }*/

                if (num_write == -1) {
                    //SPDLOG_ERROR("Send file slice failed: EAGAIN or EWOULDBLOCK, already sent {} bytes, len {}.", w_index, size);
                    //break;
                    continue;
                }
                sum_sent += num_write;
                w_index += num_write;
            } while (size - w_index > 0);
        }

        if (sum_sent - kEachSliceHeaderLen != slice->c_len()) {
            SPDLOG_ERROR("Send slice failed: compress len {} != {} bytes sent.", slice->c_len(), sum_sent);
            close(fd);
            return -1;
        }
        SPDLOG_INFO("Send slice finished. Info: index {}, len {}, c_len {}, sum sent {}(including 10 bytes Slice Headers).", slice->index(), slice->len(), slice->c_len(), sum_sent);

        close(fd);
        return 0;
    }

    static int SendFileSizeOnly(size_t file_size, unsigned short slices_num) {
        auto remote_addr = SockFactory::GetRemoteAddr(FLAGS_remote, FLAGS_port);
        auto fd = socket(AF_INET, SOCK_STREAM, 0);
        SPDLOG_INFO("fd {} for sending header.", fd);


        if (auto r = connect(fd, remote_addr, sizeof(*remote_addr)); r == -1) {
            SPDLOG_ERROR("Connect to server to send file size failed.");
            close(fd);
            return -1;
        }
        Server::SetNonBlocking(fd);

        std::ostringstream header_file_size;
        header_file_size << "S" << file_size << "#" << slices_num;
        SPDLOG_INFO("Sending FIle Header: {}, size: {}.", header_file_size.str(), header_file_size.str().size());

        auto h = header_file_size.str().c_str();
        auto w_size = header_file_size.str().size();
        size_t w_index = 0;

        do {
            auto num_write = write(fd, h+w_index, w_size-w_index);

            if (Server::IsSocketError(num_write)) {
                SPDLOG_ERROR("Send file size header failed.");
                close(fd);
                return -1;
            }

            if (num_write == 0) {
                SPDLOG_INFO("Send HEADER: Already sent {} bytes, the fd is exhausted.", w_index);
                break;
            }

            if (num_write == -1 && errno == EINTR) {
                continue;
            }

            if (num_write == -1) {
                SPDLOG_ERROR("Send HEADER failed(maybe END): EAGAIN or EWOULDBLOCKi, already sent {} bytes.", w_index);
                break;
            }
            w_index += num_write;
        } while (w_size - w_index > 0);

        SPDLOG_INFO("Sending HEADER Finished.");
        close(fd);

        return 0;
    }

    using time_point = std::chrono::steady_clock::time_point;
    static int Supervise(time_point start) {
        auto [listen_fd, epoll_fd] = Server::BindSocket(10050);

        epoll_event events[2];

        const int kSupvervisorInfoBuffSize = 512;
        char *read_buf = static_cast<char*>(malloc(kSupvervisorInfoBuffSize));

        for (;;) {
            auto num_fds = epoll_wait(epoll_fd, events, 2, -1);
            if (num_fds == -1) {
                SPDLOG_ERROR("Supervise: Epoll wait failed.");
                close(listen_fd);
                close(epoll_fd);
                return -1;
            }
            SPDLOG_INFO("{} fds are active.", num_fds);

            for (auto i = 0; i < num_fds; ++i) {
                SPDLOG_INFO("active fd {}, listen_fd {}.", events[i].data.fd, listen_fd);

                if (events[i].data.fd == listen_fd && (events[i].events & EPOLLIN)) {
                    // non-blocking accept
                    int conn_fd;
                    while((conn_fd = accept(listen_fd, nullptr, nullptr)) > 0) {
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

                        Server::SetNonBlocking(conn_fd);
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
                    SPDLOG_INFO("Supervisor res: {}", res);

                    if (res.find("E\0") != std::string::npos) {
                        time_point end = std::chrono::steady_clock::now();
                        SPDLOG_WARN("[duration:{}]", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000);
                        res = res.substr(2);
                    }
                    if (res.find("strings2:") != std::string::npos) {
                        SPDLOG_WARN("[validString:{}]", res);
                        free(read_buf);
                        close(conn_fd);
                        close(listen_fd);
                        close(epoll_fd);

                        return 0;
                    }
                }
            }
        }

        return 0;
    }
};
