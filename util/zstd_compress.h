/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: zstd_compress.h
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 11-29-2022 14:11:58
 * @version $Revision$
 *
 **************************************************************************/


#pragma once

#include "common.h"

#include "file_slice.h"

#include <zstd.h>

DECLARE_int32(jobs);

typedef struct compress_args
{
  int c_level;
#if defined(ZSTD_STATIC_LINKING_ONLY)
  ZSTD_threadPool *pool;
#endif
} compress_args_t;


class ZSTDCompressUtil {
private:
    static compress_args_t *args_;
    static int zstd_thread_num_;

    static ZSTD_CCtx * cctx_;

#if defined(ZSTD_STATIC_LINKING_ONLY)
    static ZSTD_threadPool *pool_;
#endif

public:
    static int Init() {
#if defined(ZSTD_STATIC_LINKING_ONLY)
        pool_ = ZSTD_createThreadPool(FLAGS_jobs / 2);
        if (pool_ == nullptr) {
            SPDLOG_ERROR("ZSTD create thread pool faild.");
            return -1;
        }
        SPDLOG_INFO("ZSTD uses shared thread pool of size {}.", FLAGS_jobs/2);
#else
        SPDLOG_WARN("All ZSTD threads use its own thread pool.");
#endif

        args_ = static_cast<compress_args_t*>(malloc(sizeof(compress_args_t)));
        args_ -> c_level = 1;
#if defined(ZSTD_STATIC_LINKING_ONLY)
        args_->pool = pool_;
#endif
        zstd_thread_num_ = 2;

        cctx_ = ZSTD_createCCtx();
        if (cctx_ == nullptr) {
            SPDLOG_ERROR("Create zstd context obj failed.");
            return -1;
        }

        return 0;
    }

    static int Destory() {

#if defined(ZSTD_STATIC_LINKING_ONLY)
        ZSTD_freeThreadPool(pool_);
#endif

        free(args_);
        ZSTD_freeCCtx(cctx_);

        return 0;
    }

    static void* Compress(std::shared_ptr<FileSlice> slice) {
        size_t buff_out_size = 4 * 1024 * 1024;
        size_t buff_in_size = static_cast<size_t>(6.5 * 1024 * 1024);

        if (slice->len() < 1024 * 1024) {
            buff_in_size = ZSTD_CStreamInSize();
            buff_out_size = ZSTD_CStreamOutSize();
        }
        void*  buff_out = malloc(buff_out_size);
        const void*  const buff_in = slice->GetBuf();

#if defined(ZSTD_STATIC_LINKING_ONLY)
        if (auto r = ZSTD_CCtx_refThreadPool(cctx_, args_->pool); ZSTD_isError(r)) {
            SPDLOG_ERROR("Set zstd context failed: set threadpool, {}", ZSTD_getErrorName(r));
            free(buff_out);
            return nullptr;
        }
#endif

        if (auto r = ZSTD_CCtx_setParameter(cctx_, ZSTD_c_compressionLevel, args_->c_level); ZSTD_isError(r)) {
            SPDLOG_ERROR("Set zstd context failed: set compress level, {}.", ZSTD_getErrorName(r));
            free(buff_out);
            return nullptr;
        }

        if (auto r = ZSTD_CCtx_setParameter(cctx_, ZSTD_c_checksumFlag, 0); ZSTD_isError(r)) {
            SPDLOG_ERROR("Set zstd context failed: disable checksum, {}", ZSTD_getErrorName(r));
            free(buff_out);
            return nullptr;
        }

        /*
        if (auto r = ZSTD_CCtx_setParameter(cctx_, ZSTD_c_nbWorkers, zstd_thread_num_); ZSTD_isError(r)) {
            SPDLOG_ERROR("Set zstd context failed: set nbWorkers, {}", ZSTD_getErrorName(r));
            free(buff_out);
            return nullptr;
        }*/

        size_t index = 0;
        auto slice_len = slice->len();
        size_t compressed_size = 0;

        for (;;) {
            const bool last_chunk = (index + buff_in_size  >= slice_len);

            const ZSTD_EndDirective mode = last_chunk? ZSTD_e_end : ZSTD_e_continue;

            const size_t act_buff_in_size = last_chunk ? slice_len-index : buff_in_size;

            ZSTD_inBuffer input = {static_cast<const char* const>(buff_in)+index, act_buff_in_size, 0};

            bool finished;

            do {
                /* Compress into the output buffer and Link all of the output to
                 * the FileSlice compress buffer, so we should apply a new output
                 * buffer next iteration.
                 */
                ZSTD_outBuffer output = {buff_out, buff_out_size, 0};
                const size_t remaining = ZSTD_compressStream2(cctx_, &output , &input, mode);
                if (ZSTD_isError(remaining)) {
                    SPDLOG_ERROR("ZSTD compress failed: {}", ZSTD_getErrorName(remaining));
                    free(buff_out);
                    return nullptr;
                }

                slice->LinkCompressChunk(FileSlice::Chunk{static_cast<char*>(buff_out), output.pos});
                compressed_size += output.pos;
                buff_out = malloc(buff_out_size);

                /* If we're on the last chunk we're finished when zstd returns 0,
                 * which means its consumed all the input AND finished the frame.
                 * Otherwise, we're finished when we've consumed all the input.
                 */
                finished = last_chunk? (remaining == 0) : (input.pos == input.size);
            } while (!finished);

            if (last_chunk) {
                slice->SetChunkLinksHeader(compressed_size);
                break;
            }

            index += act_buff_in_size;
        }

        return buff_out;
    }

};

ZSTD_CCtx * ZSTDCompressUtil::cctx_ = nullptr;
compress_args_t * ZSTDCompressUtil::args_ = nullptr;
int ZSTDCompressUtil::zstd_thread_num_ = 0;

#if defined(ZSTD_STATIC_LINKING_ONLY)
ZSTD_threadPool * ZSTDCompressUtil::pool_ = nullptr;
#endif
