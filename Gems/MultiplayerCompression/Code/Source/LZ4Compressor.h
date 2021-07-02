/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzNetworking/Framework/ICompressor.h>

namespace MultiplayerCompression
{
    static const char* CompressorName = "LZ4";
    static const AzNetworking::CompressorType CompressorType = aznumeric_cast<AzNetworking::CompressorType>(static_cast<AZ::u32>(AZ::Crc32(CompressorName)));

    /** 
    * Implements an LZ4 Compressor against GridMate's Compressor interface for use with the Multiplayer Gem.
    * Handles edge and error cases specific to LZ4 that are otherwise not covered in GridMate Carrier 
    * (where a Compressor is applied). 
    */
    class LZ4Compressor
        : public AzNetworking::ICompressor
    {
    public:
        AZ_CLASS_ALLOCATOR(LZ4Compressor, AZ::SystemAllocator, 0);

        LZ4Compressor() = default;

        const char* GetName() const { return CompressorName; }
        AzNetworking::CompressorType GetType() const { return CompressorType;  };

        bool Init() { return true; }
        size_t GetMaxChunkSize(size_t maxCompSize) const;
        size_t GetMaxCompressedBufferSize(size_t uncompSize) const;

        AzNetworking::CompressorError Compress(const void* uncompData, size_t uncompSize, void* compData, size_t compDataSize, size_t& compSize);
        AzNetworking::CompressorError Decompress(const void* compData, size_t compDataSize, void* uncompData, size_t uncompDataSize, size_t& consumedSize, size_t& uncompSize);
    };
}
