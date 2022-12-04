#pragma once

#include "common.h"


class MMapReader {
public:
    // del all ctors
    MMapReader() = delete;

    static std::tuple<char *, size_t> ReadArrowFile(const std::string& file_path) {
        int fd = open(file_path.c_str(), O_RDONLY);

        struct stat statbuf;
        fstat(fd, &statbuf);

        spdlog::info("open file {}, fd={}", file_path, fd);

        auto res = std::make_tuple(static_cast<char*>(mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0)), statbuf.st_size);

        close(fd);

        return res;
    }
};
