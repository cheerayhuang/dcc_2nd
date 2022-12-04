#include <iostream>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>

#include <memory>
#include <string>
#include <functional>

#include "threadpool.h"

#include <spdlog/spdlog.h>
#include "spdlog/sinks/stdout_color_sinks.h"


using namespace std::literals;

const char* IP = "127.0.0.1";
const int MAX_EVENTS = 100;
const int BUFF_SIZE = 1024;

void setnonblocking(int sockfd) {
    int opts;

    opts = fcntl(sockfd, F_GETFL);
    if(opts < 0) {
        perror("fcntl(F_GETFL)\n");
        exit(1);
    }
    opts = (opts | O_NONBLOCK);
    if(fcntl(sockfd, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)\n");
        exit(1);
    }
}

class Job {
public:

void job(int conn_fd, sockaddr_in addr, size_t addr_len, const std::string& op) {

    SPDLOG_INFO("running a job...");

    char c_ip[32]{0};
    bzero(c_ip, 20);
    inet_ntop(AF_INET, &addr.sin_addr, c_ip, 20);
    auto port = ntohs(addr.sin_port);
    std::cout << "Run a job for the clinet: " << c_ip << ":" << port << std::endl;

    auto epoll_job_fd = epoll_create1(0);
    std::cout << "epoll_job_fd: " << epoll_job_fd << std::endl;

    epoll_event ev, events[2];
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = conn_fd;

    if (epoll_ctl(epoll_job_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
    setnonblocking(conn_fd);

    std::cout << "conn_fd:" << conn_fd << std::endl;

    for(;;) {
        auto nfds = epoll_wait(epoll_job_fd, events, 1, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        std::cout << "et triggle ..." << std::endl;

        auto fd = events[0].data.fd;
        char buff[BUFF_SIZE];
        size_t read_index = 0;
        if (fd == conn_fd && (events[0].events & EPOLLIN)) {
            int n_reads = 0;
            std::cout << "reading..." << std::endl;
            while((n_reads = read(fd, buff+read_index, BUFF_SIZE)) > 0) {
            //if((n_reads = read(fd, buff+read_index, BUFF_SIZE)) > 0) {
                std::cout << "reads: " << n_reads << "bytes" << std::endl;
                read_index += n_reads;
            }
            if (n_reads < 0 && errno != EAGAIN) {
                perror("read error");
                close(fd);
                close(epoll_job_fd);
                return ;
            }
            if (n_reads < 0 && errno == EAGAIN) {
                perror("eagain?");
            }

            if (n_reads == 0) {
                perror("eof?");
                close(fd);
                close(epoll_job_fd);
                return;
            }
        }

        std::string data = op + ": "s;
        if (read_index == 0) {
            std::cout << "read_index = 0" << std::endl;
            //data += "quit"s;
        } else {
            data += std::string(buff, read_index);
        }

        std::cout << "recv:" << data << std::endl;
        std::cout << "recv:" << data.size() << std::endl;
        if (data.find("quit") != std::string::npos) {
            close(fd);
            close(epoll_job_fd);
            return;
        }

        /*
        auto data_size = data.size();
        auto n = 0;
        while (n < data_size) {
            auto n_writes = write(fd, data.c_str()+n, data_size-n);
            std::cout << "nwrites = " << n_writes << std::endl;
            if (n_writes != data_size-n) {
                if (n_writes <= 0 && errno != EAGAIN) {
                    std::cout << "here" << std::endl;
                    perror("write error");
                }
                break;
            }
            n += n_writes;
        }*/
    }

}
};

int main() {

    spdlog::set_pattern("[send %t] %+ ");
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_every(std::chrono::seconds(2));


    sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &address.sin_addr);
    address.sin_port = htons(8070);

    auto listenfd1 = socket(AF_INET, SOCK_STREAM, 0);
    bind(listenfd1, reinterpret_cast<sockaddr*>(&address), sizeof(address));

    //bzero(&address, sizeof(address));

    //address.sin_family = AF_INET;
    //inet_pton(AF_INET, IP, &address.sin_addr);
    address.sin_port = htons(8071);
    auto listenfd2 = socket(AF_INET, SOCK_STREAM, 0);
    bind(listenfd2, reinterpret_cast<sockaddr*>(&address), sizeof(address));

    auto epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    epoll_event ev1, ev2, events[MAX_EVENTS];
    ev1.events = EPOLLIN | EPOLLET;
    ev1.data.fd = listenfd1;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd1, &ev1) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
    setnonblocking(listenfd1);

    ev2. events = EPOLLIN | EPOLLET;
    ev2.data.fd = listenfd2;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd2, &ev2) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
    setnonblocking(listenfd2);

    listen(listenfd1, MAX_EVENTS);
    listen(listenfd2, MAX_EVENTS);

    auto p = std::make_unique<ThreadPool>(4);

    perror("hello");

    for(;;) {
        auto nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        std::cout << "main accept loop wait " << nfds << "fds" << std::endl;
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (auto i = 0; i < nfds; ++i) {
            sockaddr_in c_addr;
            socklen_t c_addr_len;

            int conn_fd = 0;
            if (events[i].data.fd == listenfd1) {
                while ((conn_fd = accept(
                        listenfd1,
                        reinterpret_cast<sockaddr *>(&c_addr),
                        &c_addr_len)) > 0) {

                    Job j;
                    using namespace std::placeholders;
                    auto f = std::bind(&Job::job, &j, _1, _2, _3, _4);
                    p->enqueue(f, conn_fd, c_addr, c_addr_len, "shout");
                }
                std::cout << "after accept a conn" << conn_fd << std::endl;
                if (conn_fd == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED
                            && errno != EPROTO && errno != EINTR) {
                        perror("accept");
                    }
                }
                continue;
            }
            if (events[i].data.fd == listenfd2) {
                while ((conn_fd = accept(
                        listenfd2,
                        reinterpret_cast<sockaddr *>(&c_addr),
                        &c_addr_len)) > 0) {

                    Job j;
                    using namespace std::placeholders;
                    auto f = std::bind(&Job::job, &j, _1, _2, _3, _4);
                    p->enqueue(f, conn_fd, c_addr, c_addr_len, "whisper");
                }
                std::cout << "after accept a conn" << conn_fd << std::endl;
                if (conn_fd == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED
                            && errno != EPROTO && errno != EINTR) {
                        perror("accept");
                    }
                }
            }
        }
    }

    return 0;
}
