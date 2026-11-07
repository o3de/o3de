#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

set(FILES
    "${zstd_SOURCE_DIR}/lib/common/debug.c"
    "${zstd_SOURCE_DIR}/lib/common/entropy_common.c"
    "${zstd_SOURCE_DIR}/lib/common/error_private.c"
    "${zstd_SOURCE_DIR}/lib/common/fse_decompress.c"
    "${zstd_SOURCE_DIR}/lib/common/pool.c"
    "${zstd_SOURCE_DIR}/lib/common/threading.c"
    "${zstd_SOURCE_DIR}/lib/common/xxhash.c"
    "${zstd_SOURCE_DIR}/lib/common/zstd_common.c"
    "${zstd_SOURCE_DIR}/lib/compress/fse_compress.c"
    "${zstd_SOURCE_DIR}/lib/compress/hist.c"
    "${zstd_SOURCE_DIR}/lib/compress/huf_compress.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_compress_literals.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_compress_sequences.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_compress_superblock.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_compress.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_double_fast.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_fast.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_lazy.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_ldm.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_opt.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstd_preSplit.c"
    "${zstd_SOURCE_DIR}/lib/compress/zstdmt_compress.c"
    "${zstd_SOURCE_DIR}/lib/decompress/huf_decompress.c"
    "${zstd_SOURCE_DIR}/lib/decompress/zstd_ddict.c"
    "${zstd_SOURCE_DIR}/lib/decompress/zstd_decompress_block.c"
    "${zstd_SOURCE_DIR}/lib/decompress/zstd_decompress.c"
    "${zstd_SOURCE_DIR}/lib/dictBuilder/cover.c"
    "${zstd_SOURCE_DIR}/lib/dictBuilder/divsufsort.c"
    "${zstd_SOURCE_DIR}/lib/dictBuilder/fastcover.c"
    "${zstd_SOURCE_DIR}/lib/dictBuilder/zdict.c"
    "${zstd_SOURCE_DIR}/lib/legacy/zstd_v04.c"
    "${zstd_SOURCE_DIR}/lib/legacy/zstd_v05.c"
    "${zstd_SOURCE_DIR}/lib/legacy/zstd_v06.c"
    "${zstd_SOURCE_DIR}/lib/legacy/zstd_v07.c"
)

if(zstd_asm)
    list(APPEND FILES "${zstd_SOURCE_DIR}/lib/decompress/huf_decompress_amd64.S")
endif()
