/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzNetworking/Framework/ICompressor.h>
#include <AzCore/Casting/numeric_cast.h>

namespace MultiplayerCompression
{
    static const char* CompressorName = "LZ4";
    static const AzNetworking::CompressorType CompressorType = aznumeric_cast<AzNetworking::CompressorType>(static_cast<AZ::u32>(AZ::Crc32(CompressorName)));

    /** 
    * Implements an LZ4 Compressor against Multiplayer's Compressor interface for use with AzNetworking.
    * Handles edge and error cases specific to LZ4 that are otherwise not covered in AzNetworking 
    * (where a Compressor is applied). 
    */
    class LZ4Compressor
        : public AzNetworking::ICompressor
    {
    public:
        AZ_CLASS_ALLOCATOR(LZ4Compressor, AZ::SystemAllocator);

        LZ4Compressor() = default;

        const char* GetName() const { return CompressorName; }
        AzNetworking::CompressorType GetType() const override { return CompressorType;  };

        bool Init() override { return true; }
        size_t GetMaxChunkSize(size_t maxCompSize) const override;
        size_t GetMaxCompressedBufferSize(size_t uncompSize) const override;

        AzNetworking::CompressorError Compress(const void* uncompData, size_t uncompSize, void* compData, size_t compDataSize, size_t& compSize) override;
        AzNetworking::CompressorError Decompress(const void* compData, size_t compDataSize, void* uncompData, size_t uncompDataSize, size_t& consumedSize, size_t& uncompSize) override;
    };
}
