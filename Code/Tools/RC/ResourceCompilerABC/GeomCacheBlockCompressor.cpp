/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "ResourceCompilerABC_precompiled.h"
#include <zlib.h>
#include "GeomCacheBlockCompressor.h"

#include "zlib.h"
#include "lz4.h"
#include "lz4hc.h"
#include <zstd.h>

bool GeomCacheDeflateBlockCompressor::Compress(std::vector<char>& input, std::vector<char>& output)
{
    const size_t uncompressedSize = input.size();

    // Reserve buffer. zlib maximum overhead is 5 bytes per 32KB block + 6 bytes fixed.
    const size_t maxCompressedSize = uncompressedSize + ((uncompressedSize / 32768) + 1) * 5 + 6;
    output.resize(maxCompressedSize);

    unsigned long compressedSize = 0;

    // Deflate
    z_stream stream;
    int error;

    stream.next_in = (Bytef*)&input[0];
    stream.avail_in = (uInt)uncompressedSize;

    stream.next_out = (Bytef*)&output[0];
    stream.avail_out = (uInt)maxCompressedSize;

    stream.zalloc = Z_NULL;
    stream.zfree  = Z_NULL;
    stream.opaque = Z_NULL;

    error = deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);
    if (error != Z_OK)
    {
        return false;
    }

    error = deflate(&stream, Z_FINISH);
    if (error != Z_STREAM_END)
    {
        deflateEnd(&stream);
        return false;
    }

    compressedSize = stream.total_out;

    error = deflateEnd(&stream);
    if (error != Z_OK)
    {
        return false;
    }

    if (compressedSize == 0)
    {
        return false;
    }

    // Resize output to final, compressed size
    output.resize(compressedSize);

    return true;
}

bool GeomCacheLZ4HCBlockCompressor::Compress(std::vector<char>& input, std::vector<char>& output)
{
    const size_t uncompressedSize = input.size();

    // Reserve compress buffer
    const size_t maxCompressedSize = LZ4_compressBound(uncompressedSize);
    output.resize(maxCompressedSize);

    // Compress
    int compressedSize = LZ4_compressHC(input.data(), output.data(), uncompressedSize);

    if (compressedSize == 0)
    {
        return false;
    }

    // Resize output to final, compressed size
    output.resize(compressedSize);

    return true;
}

bool GeomCacheZStdBlockCompressor::Compress(std::vector<char>& input, std::vector<char>& output)
{
    const size_t uncompressedSize = input.size();

    // Reserve compress buffer
    const size_t maxCompressedSize = ZSTD_compressBound(uncompressedSize);
    output.resize(maxCompressedSize);

    // Compress
    int compressedSize = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), 1);

    if (ZSTD_isError(compressedSize))
    {
        return false;
    }

    // Resize output to final, compressed size
    output.resize(compressedSize);

    return true;
}
