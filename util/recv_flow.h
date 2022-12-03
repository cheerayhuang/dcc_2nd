/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: recv_flow.h
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 12-01-2022 21:19:10
 * @version $Revision$
 *
 **************************************************************************/

#pragma once

#include "common.h"
#include "mmap_writer.h"
#include "sock_factory.h"
#include "thread_pool.h"
#include "zstd_decompress.h"

DECLARE_int32(jobs);
DECLARE_string(file_path);
DECLARE_string(remote);

using namespace std::literals;

class Server {


public:

    Server() = delete;

    //static const int kListenPort = 10049;
    static const std::string IP;
    static const int kMaxEvents = 128;

    // "Sxxxxxxxxxxx#xxx"
    static const int kFileSliceHeaderLen = 16;
    static const int kEachSliceHeaderLen = 10;

    struct FileSliceInfo {
        unsigned short index_;
        size_t c_len_;
        size_t recv_len_;
        size_t len_;
        size_t offset_;
        char* buf_;
    };

    static std::map<unsigned short, std::shared_ptr<FileSliceInfo>> mmap_;
    static std::atomic_short slices_th_num_;
    static std::vector<std::future<int>> slices_th_res_;

    static size_t split_len_;
    static size_t file_size_;
    static size_t slices_num_;
    static size_t reads_max_bytes_;
    static std::atomic<char*> start_;

    static std::unique_ptr<ThreadPool> p_;

    static int SetNonBlocking(int fd) {
        int opts;

        opts = fcntl(fd, F_GETFL);
        if(opts < 0) {
            SPDLOG_ERROR("Invoke fcntl(F_GETFL) failed.");
            return -1;
        }

        opts = (opts | O_NONBLOCK);
        if(fcntl(fd, F_SETFL, opts) < 0) {
            SPDLOG_ERROR("Invoke fcntl(F_SETFL) failed.");
            return -1;
        }

        return 0;
    }

    static std::tuple<int, int> BindSocket(int listen_port) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, IP.c_str(), &addr.sin_addr);
        addr.sin_port = htons(listen_port);

        auto listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        auto epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            SPDLOG_ERROR("Create epoll fd failed.");
            return std::make_tuple(-1, -1);
        }

        epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = listen_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
            SPDLOG_ERROR("Calling epoll_ctl failed: add listen fd.");
            return std::make_tuple(-1, -1);
        }

        if (SetNonBlocking(listen_fd) != 0) {
            return std::make_tuple(-1, -1);
        }

        listen(listen_fd, kMaxEvents);

        return std::make_tuple(listen_fd, epoll_fd);
    }

    static int Supervise() {
        auto fd = socket(AF_INET, SOCK_STREAM, 0);
        auto remote_addr = SockFactory::GetRemoteAddr(FLAGS_remote, 10050);

        if (auto r = connect(fd, remote_addr, sizeof(*remote_addr)); r == -1) {
            SPDLOG_ERROR("Connect to client for supervisor failed.");
            close(fd);
            return -1;
        }

        do {
        } while(slices_th_num_ == slices_num_+1);

        for (auto&& f : slices_th_res_) {
            if (f.get() != 0) {
                SPDLOG_WARN("some Recv threads return non-zero result.");
            }
        }
        write(fd, "END.\n"s.c_str(), 5);
        //VerifyArrowFile();

        return 0;
    }

    static int Run(int listen_port = 10049) {
        auto [listen_fd, epoll_fd] = BindSocket(listen_port);
        p_ = std::make_unique<ThreadPool>(FLAGS_jobs+1);

        p_->enqueue(Supervise);

        epoll_event events[2];
        // main loop to supervise listen fd
        for (;;) {
            auto num_fds = epoll_wait(epoll_fd, events, kMaxEvents, -1);
            if (num_fds == -1) {
                SPDLOG_ERROR("Epoll wait failed.");
                return -1;
            }
            SPDLOG_INFO("{} fds are active.", num_fds);

            for (auto i = 0; i < num_fds; ++i) {
                SPDLOG_INFO("active fd {} == listen_fd {}.", events[i].data.fd, listen_fd);

                // non-blocking accept
                while(auto conn_fd = accept(listen_fd, nullptr, nullptr) > 0) {
                     slices_th_res_.push_back(p_ ->enqueue(Recv, conn_fd));
                     slices_th_num_++;
                }
            }
        }

        return 0;
    }

    static int Recv(int conn_fd) {
        auto epoll_recv_fd = epoll_create1(0);
        epoll_event ev, events[2];
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn_fd;

        if (epoll_ctl(epoll_recv_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
            SPDLOG_ERROR("Calling epoll_ctl failed: add conn fd.");
            close(conn_fd);
            close(epoll_recv_fd);
            return  -1;
        }
        SetNonBlocking(conn_fd);

        // recv loop
        auto is_first_recv = true;
        unsigned short index = 0;

        char *read_buf = nullptr;

        for (;;) {
            auto num_fds = epoll_wait(epoll_recv_fd, events, 2, -1);
            if (num_fds == -1) {
                SPDLOG_ERROR("Epoll wait failed in recv loop.");
                return -1;
            }

            SPDLOG_INFO("active fd {} == conn_fd {}.", events[0].data.fd, conn_fd);
            if (!(events[0].events & EPOLLIN)) {
                SPDLOG_WARN("Not EPOLLIN events, this could not be happenend.");
                continue;
            }

            if (is_first_recv && split_len_ == 0 && start_ == nullptr) {
                reads_max_bytes_ = kFileSliceHeaderLen + 1;
                read_buf = static_cast<char*>(malloc(reads_max_bytes_));
            }

            ssize_t num_read = 0;
            ssize_t read_index = 0;
            do {
                num_read = read(conn_fd, read_buf + read_index, reads_max_bytes_ - read_index);
                if (IsSocketError(num_read)) {
                    SPDLOG_ERROR("read file slice failed. perror():");
                    perror("\t read() failed");
                    free(read_buf);
                    close(conn_fd);
                    close(epoll_recv_fd);
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
            } while(reads_max_bytes_ - read_index > 0 && num_read != 0);
            num_read = read_index;
            if (num_read < reads_max_bytes_) {
                SPDLOG_INFO("the conn_fd for reading is exhausted.");
            }

            if (is_first_recv && split_len_ == 0 && start_ == nullptr) {
                std::string header_str{read_buf, static_cast<size_t>(num_read)};
                SPDLOG_INFO("recv HEADER: {}", header_str);

                if (header_str[0] == 'S')  {
                    auto pos = header_str.find("#");
                    if (pos == std::string::npos) {
                        SPDLOG_ERROR("HEADER format is not correct.");
                        free(read_buf);
                        close(conn_fd);
                        close(epoll_recv_fd);
                        return -1;
                    }
                    file_size_ = std::stoul(header_str.substr(1, pos-1));
                    slices_num_ = std::stoul(header_str.substr(pos+1));
                    split_len_ = file_size_ / slices_num_;

                    if (split_len_ > 1024*1024) {
                        reads_max_bytes_ = 4 * 1024 * 1024 + 1;
                    } else {
                        reads_max_bytes_ = 128 * 1024;
                    }
                    read_buf = static_cast<char*>(malloc(reads_max_bytes_));

                    start_.store(MMapWriter::MMapOutputArrowFile(FLAGS_file_path, file_size_));
                } else {
                    SPDLOG_ERROR("HEADER format is not correct.");
                    free(read_buf);
                    close(conn_fd);
                    close(epoll_recv_fd);
                    return -1;
                }

                if (num_read == kFileSliceHeaderLen+1) {
                    SPDLOG_WARN("recv 16+bytes in HEADER, 17th byte is {}, int {}", header_str[16], static_cast<unsigned int>(header_str[16]));
                }
                free(read_buf);
                close(conn_fd);
                close(epoll_recv_fd);
                return 0;
            }

            char *in_buf = nullptr;
            char *out_buf = nullptr;
            size_t in_buf_size, out_buf_size;

            if (is_first_recv) {
                // unmarshal file slice header.
                // "index: u_short, len: u_int, c_len: u_int"
                is_first_recv = false;

                uint16_t h2 = 0;
                uint32_t h4 = 0;
                memcpy(read_buf, &h2, sizeof(uint16_t));
                memcpy(read_buf+2, &h4, sizeof(uint32_t));

                index = ntohs(h2);
                auto f_info = std::make_shared<FileSliceInfo>();
                f_info->index_ = index;
                f_info->offset_ = 0;
                f_info->recv_len_ = 0;
                f_info->len_ = static_cast<size_t>(ntohl(h4));

                memcpy(read_buf+6, &h4, sizeof(uint32_t));
                f_info->c_len_ = static_cast<size_t>(ntohl(h4));

                f_info->buf_ = start_ + index * split_len_;

                mmap_[index] = f_info;
                if (num_read == kEachSliceHeaderLen) {
                    continue;
                }
                in_buf = read_buf + kEachSliceHeaderLen;
                in_buf_size = num_read - kEachSliceHeaderLen;
                mmap_[index]->recv_len_ += in_buf_size;
            }

            if (in_buf == nullptr) {
                in_buf = read_buf;
                in_buf_size = num_read;
                mmap_[index]->recv_len_ += num_read;
            }
            if (out_buf == nullptr) {
                out_buf = mmap_[index]->buf_ + mmap_[index]->offset_;
                out_buf_size = static_cast<size_t>(in_buf_size / 0.615);
            }


            auto act_wr_size = ZSTDDecompressUtil::Decompress(
                in_buf,
                in_buf_size,
                out_buf,
                out_buf_size
            );
            if (act_wr_size == -1) {
                SPDLOG_ERROR("Decompress stream failed. File Slice Info: index {}, offset {}, split_len {}, len {}, c_len {}, recv_len{}", mmap_[index]->index_, mmap_[index]->offset_, split_len_, mmap_[index]->len_, mmap_[index]->c_len_, mmap_[index]->recv_len_);
                free(read_buf);
                close(conn_fd);
                close(epoll_recv_fd);
                return -1;
            }

            MMapWriter::MMapSync(mmap_[index]->buf_ + mmap_[index]->offset_, act_wr_size, MS_ASYNC);
            mmap_[index]->offset_ += act_wr_size;
            SPDLOG_INFO("File Slice Info: index {}, offset {}, split_len {}, len {}, c_len {}, recv_len{}", index, mmap_[index]->offset_, split_len_, mmap_[index]->len_, mmap_[index]->c_len_, mmap_[index]->recv_len_);

            if(mmap_[index]->offset_ == mmap_[index]->len_ && mmap_[index]->recv_len_ == mmap_[index]->c_len_) {
                SPDLOG_INFO("Recv file slice {} Finished.", index);
                break;
            }
        }

        return 0;
    }

    static bool IsSocketError(ssize_t r) {
        if (r == -1 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            return true;
        }
        return false;
    }

};

const std::string Server::IP{"0.0.0.0"};

std::unique_ptr<ThreadPool> Server::p_ = nullptr;

std::atomic<char*> Server::start_ = nullptr;
size_t Server::split_len_ = 0;

std::map<unsigned short, std::shared_ptr<Server::FileSliceInfo>> Server::mmap_ {};
std::atomic_short Server::slices_th_num_ = 0;
std::vector<std::future<int>> slices_th_res_{};
