#pragma once

#include "common.h"

class FileSlice {

public:
    struct Chunk {
        char *c_;
        const size_t len_;

        Chunk(char*c, size_t len)
            : c_(c), len_(len) {
        }

        Chunk(Chunk&& c)
            :c_(c.c_), len_(c.len_) {
            c.c_ = nullptr;
        }
    };

public:
    FileSlice(const char*buf, unsigned short index, size_t len, size_t split_len);

    FileSlice(const FileSlice&) = delete;
    FileSlice(FileSlice&& f)
        : slice_(f.slice_), index_(f.index_), len_(f.len_), split_len_(f.split_len_), c_len_(f.c_len_) {

        compress_buf_ = std::move(f.compress_buf_);
        f.slice_ = nullptr;
    }

    ~FileSlice() {
        for (auto && i : compress_buf_) {
            free(i.c_);
        }
    }

    const char* GetBuf() const;

    unsigned short index() const;
    size_t len() const;
    size_t c_len() const;


    const std::list<Chunk>& compress_buf() const {
        return compress_buf_;
    }

    void LinkCompressChunk(Chunk&& chunk) {
        if (compress_buf_.empty()) {
            // short index, unsigned int len_, unsigned int c_len_
            auto h_len = sizeof(uint16_t) + 2 * sizeof(uint32_t);
            auto b = static_cast<char*>(malloc(h_len));
            memset(b, 0, h_len);

            auto i = htons(index_);
            auto l = htonl(static_cast<uint32_t>(len_));
            memcpy(b, &i, sizeof(uint16_t));
            memcpy(b+2, &l, sizeof(uint32_t));

            compress_buf_.emplace_back(b, h_len);
        }

        compress_buf_.push_back(std::move(chunk));
    }

    void SetChunkLinksHeader(size_t compressed_size) {
        c_len_ = compressed_size;

        auto l = htonl(static_cast<uint32_t>(compressed_size));
        auto h = compress_buf_.begin();
        memcpy(h->c_ + 6, &l, sizeof(uint32_t));
    }

    static std::vector<std::shared_ptr<FileSlice>> SplitFile(char* mmap_file_buf, size_t buf_size);


private:

    const char* slice_;
    const unsigned short index_;
    const size_t len_;
    const size_t split_len_;
    size_t c_len_;

    std::list<Chunk> compress_buf_{};
};


FileSlice::FileSlice(const char* buf, unsigned short index, size_t len, size_t split_len)
    : slice_(buf), index_(index), len_(len), split_len_(split_len), c_len_(0) {
    compress_buf_.clear();
}

const char *FileSlice::GetBuf() const {
    return slice_ + index_ * split_len_;
}

unsigned short FileSlice::index() const {
    return index_;
}

size_t FileSlice::len() const {
    return len_;
}

size_t FileSlice::c_len() const {
    return c_len_;
}

DECLARE_int64(segment_size);
DECLARE_int32(jobs);

// this func can modify global parameters
std::vector<std::shared_ptr<FileSlice>> FileSlice::SplitFile(char * mmap_file_buf, size_t buf_size) {
    if (FLAGS_segment_size == 0) {
        FLAGS_segment_size = buf_size / FLAGS_jobs;
    }

    if (FLAGS_segment_size > std::numeric_limits<int>::max()) {
        FLAGS_segment_size = std::numeric_limits<int>::max();
        FLAGS_jobs = buf_size / FLAGS_segment_size;
    }

    auto rest = buf_size % FLAGS_segment_size;

    unsigned short index = 0;
    std::vector<std::shared_ptr<FileSlice>> res;
    size_t f_index = 0;
    do {
        size_t slice_size = FLAGS_segment_size;
        if (f_index + slice_size + rest == buf_size) {
            slice_size += rest;
        }

        res.push_back(std::make_shared<FileSlice>(mmap_file_buf, index++, slice_size, FLAGS_segment_size));

        f_index += slice_size;
    } while (f_index < buf_size);

    return res;
}
