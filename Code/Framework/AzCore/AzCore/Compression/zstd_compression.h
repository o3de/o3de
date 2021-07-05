/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#define ZSTD_STATIC_LINKING_ONLY

#include <zstd.h>

namespace AZ
{
    class IAllocator;
    class IAllocatorAllocate;

    class ZStd
    {
    public:
        ZStd(IAllocatorAllocate* workMemAllocator = 0);
        ~ZStd();

        enum FlushType
        {
            // Mapped to the Z_LIB flush values, please reference the ZLib documentation.
            FT_NO_FLUSH = 0,
            FT_PARTIAL_FLUSH,
            FT_SYNC_FLUSH,
            FT_FULL_FLUSH,
            FT_FINISH,
            FT_BLOCK,
            FT_TREES,
        };

        /*
        Description of the zstd data format can be found here:
        https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md

        Additional information here:
        https://tools.ietf.org/id/draft-kucherawy-dispatch-zstd-00.html
        */

        using Header = AZ::u32;     ///< Typedef for the  byte zstd header.

        void StartCompressor(unsigned int compressionLevel = 1);
        bool IsCompressorStarted() const;
        void StopCompressor();
        void ResetCompressor();

        void StartDecompressor();
        bool IsDecompressorStarted() const;
        void StopDecompressor();
        /// If you will use seek/sync points we require that you pass the header since the reset will reset all states and you can't really continue (unless from the start).
        void ResetDecompressor(Header* header = nullptr);

        //////////////////////////////////////////////////////////////////////////
        // Compressor
        unsigned int GetMinCompressedBufferSize(unsigned int sourceDataSize);

        unsigned int Compress(const void* data, unsigned int& dataSize, void* compressedData, unsigned int compressedDataSize, FlushType flushType = FT_NO_FLUSH);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Decompressor
        unsigned int Decompress(const void* compressedData, unsigned int compressedDataSize, void* outputData, unsigned int outputDataSize, size_t* sizeOfNextBlock);
        //////////////////////////////////////////////////////////////////////////
    private:
        static void* AllocateMem(void* userData, size_t size);
        static void  FreeMem(void* userData, void* address);

        void         SetupDecompressHeader(Header header);

        ZSTD_CStream*   m_streamCompression;
        ZSTD_DStream*   m_streamDecompression;
        IAllocatorAllocate*     m_workMemoryAllocator;
        ZSTD_inBuffer   m_inBuffer;
        ZSTD_outBuffer  m_outBuffer;
        size_t          m_nextBlockSize;
        unsigned int    m_compressedBufferIndex;
    };
};
