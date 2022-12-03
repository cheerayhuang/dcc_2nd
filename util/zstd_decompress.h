/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: zstd_decomporess.h
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 12-03-2022 09:03:43
 * @version $Revision$
 *
 **************************************************************************/

#pragma once

#include "common.h"

#include <zstd.h>


class ZSTDDecompressUtil {

private:
    static ZSTD_DCtx *dctx_;

public:

    static int Init() {
        dctx_ = ZSTD_createDCtx();
        if (dctx_ == nullptr) {
            SPDLOG_ERROR("Create zstd D context obj failed.");
            return -1;
        }

        return 0;
    }

    static int Destory() {
        ZSTD_freeDCtx(dctx_);

        return 0;
    }


    static int Decompress(char *in_buf, size_t in_buf_size, char* out_buf, size_t out_buf_size) {
        ZSTD_inBuffer input = {in_buf, in_buf_size, 0};
        size_t out_index = 0;
        while (input.pos < input.size) {
            ZSTD_outBuffer output = {out_buf+out_index, out_buf_size, 0};
            size_t const ret = ZSTD_decompressStream(dctx_, &output, &input);
            if (ZSTD_isError(ret)) {
                    SPDLOG_ERROR("ZSTD decompress failed: {}", ZSTD_getErrorName(ret));
                    return -1;
            }
            out_index += output.pos;
        }

        return out_index;
    }

};

ZSTD_DCtx* ZSTDDecompressUtil::dctx_ = nullptr;
