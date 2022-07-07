/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LZ4Compressor.h"

#include <lz4.h>
#include <lz4hc.h>

namespace MultiplayerCompression
{
    size_t LZ4Compressor::GetMaxChunkSize(size_t maxCompSize) const
    {
        return maxCompSize;
    }

    size_t LZ4Compressor::GetMaxCompressedBufferSize(size_t uncompSize) const
    {
        return LZ4_compressBound(static_cast<int>(uncompSize));
    }

    AzNetworking::CompressorError LZ4Compressor::Compress
    (
        const void* uncompData,
        size_t uncompSize,
        void* compData,
        [[maybe_unused]] size_t compDataSize,
        size_t& compSize
    )
    {
        if (uncompData == nullptr)
        {
            // LZ4 actually never checks for this
            AZ_Warning("Multiplayer Compressor", false, "Input buffer is uninitialized");
            return AzNetworking::CompressorError::Uninitialized;
        }

        if (compData == nullptr)
        {
            // LZ4 actually never checks for this
            AZ_Warning("Multiplayer Compressor", false, "Output buffer is uninitialized");
            return AzNetworking::CompressorError::Uninitialized;
        }

        const int compWorstCaseSize = LZ4_compressBound(static_cast<int>(uncompSize));
        if (compWorstCaseSize == 0)
        {
            AZ_Warning("Multiplayer Compressor", false, "Input size (%lu) passed to Compress() is greater than max allowed (%lu)", uncompSize, LZ4_MAX_INPUT_SIZE);
            return AzNetworking::CompressorError::InsufficientBuffer;
        }

        AZ_Warning("Multiplayer Compressor", compDataSize >= compWorstCaseSize, "Outbuffer size (%lu B) passed to Compress() is less than estimated worst case (%lu B)", compDataSize, compWorstCaseSize);

        // Note that this returns a non-negative int so we are narrowing into a size_t here
        compSize = LZ4_compress_HC(
            reinterpret_cast<const char*>(uncompData), 
            reinterpret_cast<char*>(compData), 
            static_cast<int>(uncompSize),
            static_cast<int>(compDataSize),
            0);

        if (compSize == 0)
        {
            // LZ4_compress_HC returns a zero value for corrupt data and insufficient buffer
            AZ_Warning("Multiplayer Compressor", false, "Compression failed for uncompSize:(%lu B) compDataSize:(%lu B) compSize:(%lu B)", uncompSize, compDataSize, compSize);
            return AzNetworking::CompressorError::CorruptData;
        }

        return AzNetworking::CompressorError::Ok;
    }

    AzNetworking::CompressorError LZ4Compressor::Decompress(const void* compData, size_t compDataSize, void* uncompData, size_t uncompDataSize, size_t& consumedSizeOut, size_t& uncompSizeOut)
    {
        if (uncompData == nullptr)
        {
            // LZ4 actually never checks for this
            AZ_Warning("Multiplayer Compressor", false, "Input buffer is uninitialized");
            return AzNetworking::CompressorError::Uninitialized;
        }

        if (compData == nullptr)
        {
            // LZ4 actually never checks for this
            AZ_Warning("Multiplayer Compressor", false, "Output buffer is uninitialized");
            return AzNetworking::CompressorError::Uninitialized;
        }

        const int uncompSize = LZ4_decompress_safe(reinterpret_cast<const char*>(compData), reinterpret_cast<char*>(uncompData), static_cast<int>(compDataSize), static_cast<int>(uncompDataSize));
        consumedSizeOut = compDataSize;

        if (uncompSize < 0)
        {
            // LZ4_decompress_safe returns a negative value for corrupt data and insufficient buffer
            AZ_Warning("Multiplayer Compressor", false, "Decompression failed for compDataSize:(%lu B) uncompDataSize:(%lu B) uncompSize:(%d B)", compDataSize, uncompDataSize, uncompSize);
            return AzNetworking::CompressorError::CorruptData;
        }
        // Assign into the outbound size_t after validating the negative error case
        uncompSizeOut = uncompSize;

        return AzNetworking::CompressorError::Ok;
    }
}
