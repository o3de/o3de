/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_COMPRESSOR_H
#define AZCORE_COMPRESSOR_H

#include <AzCore/base.h>

struct z_stream_s;

namespace AZ
{
    class IAllocator;

    /**
     * The most well known and used compression algorithm. It gives the best compression ratios even on level 1,
     * the speed and memory usage can be an issue. If you want detailed control over the compressed stream, include
     * "AzCore/compression/zlib/zlib.h" and do it yourself!
     */
    class ZLib
    {
    public:
        ZLib(IAllocator* workMemAllocator = 0);
        ~ZLib();

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

        typedef AZ::u16 Header;     ///< Typedef for the 2 byte zlib header.

        /// Must be called before we can compress. Compression level can vary from [0 - no compression to 9 - best compression]. Default is 9.
        /// Compression level results from a test input stream of ~26MB comprised of a mix of string and binary data:
        ///     Level   Compressed Size     Time(ms)
        ///     1       ~1.9MB              55ms
        ///     2       ~1.8MB              53ms
        ///     3       ~1.7MB              45ms
        ///     4       ~1.7MB              230ms
        ///     5       ~1.6MB              225ms
        ///     6       ~1.5MB              325ms
        ///     7       ~1.5MB              387ms
        ///     8       ~1.4MB              827ms
        ///     9       ~1.4MB              858ms
        void StartCompressor(unsigned int compressionLevel = 9);
        bool IsCompressorStarted() const        { return m_strDeflate != 0; }
        void StopCompressor();
        void ResetCompressor();

        /// Must be called before we can decompress. Hdr is optional hdr structure that is stored at the begin of the stream and should be passed to the ResetDecompresor.
        void StartDecompressor(Header* hdr = NULL);
        bool IsDecompressorStarted() const      { return m_strInflate != 0; }
        void StopDecompressor();
        /// If you will use seek/sync points we require that you pass the header since the reset will reset all states and you can't really continue (unless from the start).
        void ResetDecompressor(Header* header = NULL);

        //////////////////////////////////////////////////////////////////////////
        // Compressor
        /// Return compressed buffer minimal size for the given source size. compressedDataSize in compress must be at least that size, otherwise compression will fail.
        unsigned int GetMinCompressedBufferSize(unsigned int sourceDataSize);
        /**
         * Compressed data from the data buffer into compressedData buffer.
         * If compressedData is NOT big enough the left over size will be returned in "dataSize". If dataSize is 0 all data has been compressed.
         * \returns number of bytes written in compressedData.
         */
        unsigned int Compress(const void* data, unsigned int& dataSize, void* compressedData, unsigned int compressedDataSize, FlushType flushType = FT_NO_FLUSH);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Decompressor
        unsigned int Decompress(const void* compressedData, unsigned int compressedDataSize, void* data, unsigned int& dataSize, FlushType flushType = FT_NO_FLUSH);
        //////////////////////////////////////////////////////////////////////////
    private:
        static void* AllocateMem(void* userData, unsigned int items, unsigned int size);
        static void  FreeMem(void* userData, void* address);

        void         SetupDecompressHeader(Header header);

        z_stream_s* m_strDeflate;
        z_stream_s* m_strInflate;
        IAllocator* m_workMemoryAllocator;
    };
}

#endif // AZCORE_COMPRESSOR_H
#pragma once
