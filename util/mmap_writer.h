/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: mmap_writer.h
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 12-01-2022 09:53:17
 * @version $Revision$
 *
 **************************************************************************/

#pragma once

#include "common.h"

class MMapWriter {
public:
    MMapWriter() = delete;

    static char *MMapOutputArrowFile(const std::string& file_name, size_t file_size) {
        auto fd = open(file_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);

        auto pos = lseek(fd, file_size-1, SEEK_SET);
        write(fd, "", 1);
        SPDLOG_INFO("gen output file: {}, {} bytes", file_name, file_size);

        char *start = static_cast<char*>(mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

        close(fd);

        return start;
    }

    static void *MMapWriting(char *start, char* buf, size_t pos, size_t count, size_t file_size, int mmap_flags=MS_SYNC) {
        auto p = static_cast<char*>(memcpy(start+pos, buf, count));
        msync(start, file_size, mmap_flags);

        return p;
    }

    static void MMapSync(char *start, size_t size, int mmap_flags=MS_SYNC) {
        msync(start, size, mmap_flags);
    }

    static void UnMapOutputArrowFile(char* start, size_t file_size) {
        munmap(start, file_size);
    }
};
