/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace CompressionCodec
{

    enum class Codec : uint8_t
    {
        INVALID = static_cast<uint8_t>(-1),
        ZLIB = 0,
        ZSTD,
        LZ4,
        NUM_CODECS
    };

    inline constexpr Codec s_AllCodecs[] = { Codec::ZLIB, Codec::ZSTD, Codec::LZ4 };

    inline bool CheckMagic(const void* pCompressedData, const uint32_t magicNumber, const uint32_t magicSkippable)
    {
        
        uint32_t compressedMagic = 0;
        //If the address is 4bytes aligned, then it is safe to dereference as a 4byte integral.
        if ((reinterpret_cast<uintptr_t>(pCompressedData) & 0x3) == 0)
        {
            compressedMagic = *reinterpret_cast<const uint32_t*>(pCompressedData);
        }
        else
        {
            //We should read one byte at a time to avoid alignment issues.
            const uint8_t* pCompressedBytes = static_cast<const uint8_t*>(pCompressedData);
            compressedMagic = static_cast<uint32_t>(pCompressedBytes[0]) | (static_cast<uint32_t>(pCompressedBytes[1]) << 8) |
                (static_cast<uint32_t>(pCompressedBytes[2]) << 16) | (static_cast<uint32_t>(pCompressedBytes[3]) << 24);
        }
        if (compressedMagic == magicNumber)
        {
            return true;
        }
        if ((compressedMagic & 0xFFFFFFF0) == magicSkippable)
        {
            return true;
        }
        return false;
    }


    inline bool TestForLZ4Magic(const void* pCompressedData)
    {
        constexpr uint32_t lz4MagicNumber = 0x184D2204;
        constexpr uint32_t lz4MagicSkippable = 0x184D2A50;
        return CheckMagic(pCompressedData, lz4MagicNumber, lz4MagicSkippable);
    }

    inline bool TestForZSTDMagic(const void* pCompressedData)
    {
        constexpr uint32_t zstdMagicNumber = 0xFD2FB528;
        constexpr uint32_t zstdMagicSkippable = 0x184D2A50; //Same as LZ4
        return CheckMagic(pCompressedData, zstdMagicNumber, zstdMagicSkippable);
    }

};
