/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_COMPRESSOR_ZSTD_H
#define AZCORE_COMPRESSOR_ZSTD_H

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/IO/Compressor.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Compression/zstd_compression.h>

namespace AZ
{
    namespace IO
    {
        /**
         * Header stored after the standard compression header.
         * This structure is padded and aligned don't change members.
         */
        struct CompressorZStdHeader
        {
            AZ::u32     m_numSeekPoints;    ///< Number of seek points located at the end of the stream.
        };

        /**
         * Seek points are stored at the end of the archive, we can have
         * 0..N seek points. If 0 the entire file is one seek point.
         */
        struct CompressorZStdSeekPoint
        {
            AZ::u64     m_compressedOffset;         ///< Location in the compressed stream of the sync point.
            AZ::u64     m_uncompressedOffset;       ///< Location in the decompressed stream.
        };

        /**
         * ZStd compressor per stream data.
         */
        class CompressorZStdData
            : public CompressorData
        {
        public:
            AZ_CLASS_ALLOCATOR(CompressorZStdData, AZ::SystemAllocator);

            CompressorZStdData(IAllocator* zstdMemAllocator = 0)
            {
                m_zstd = zstdMemAllocator;
            }

            ZStd              m_zstd;
            AZ::u64           m_decompressNextOffset{};     ///< Next offset in the compressed stream for decompressing.
            AZ::u64           m_decompressLastOffset{};     ///< Last valid offset in the compressed stream of the compressed data. Used only when we decompress.
            unsigned char*    m_decompressedCache{};      ///< Decompressed stream cache.
            unsigned int      m_decompressedCacheDataSize{};   ///< Number of valid bytes in the decompressed cache.
            ZStd::Header      m_zstdHeader;                       ///< Stored 2 bytes header of the zlib when we reset the decompressor.
            union
            {
                AZ::u64       m_decompressedCacheOffset{};  ///< Used when decompressing. Decompressed cache is the data offset in the uncompressed data stream.
                AZ::u64       m_autoSeekSize;             ///< Used when compressing to define auto seek point window.
            };

            using SeekPointArray = AZStd::vector<CompressorZStdSeekPoint> ;
            SeekPointArray    m_seekPoints;               ///< List of seek points for the archive, we must have at least one!
        };

        class CompressorZStd
            : public Compressor
        {
        public:
            AZ_CLASS_ALLOCATOR(CompressorZStd, AZ::SystemAllocator);

            /**
             * \param decompressionCachePerStream cache of decompressed data stored per stream, the more streams you have open the more memory it will use,
             * You can use 0 size too to save memory, but every read will happen with decompression from the closest seek point. That might be fine if we
             * use stream read cache and it matches the size of the seek points (you will need to make sure of that), otherwise the extra cache can be useful.
             * We can convert this system to use pools of caches, but as of now this should be fine.
             * \param dataBufferSize we have one compressor per device, only one stream can read/write at a time and the data buffer is shared for IO operations,
             * the buffer is refCounted and existing only when we read/write compressed streams.
             */
            CompressorZStd(unsigned int decompressionCachePerStream = 64* 1024, unsigned int dataBufferSize = 128* 1024);

            ~CompressorZStd() override;

            /// Return compressor type id.
            static AZ::u32 TypeId();
            AZ::u32 GetTypeId() const override { return TypeId(); }
            /// Called when we open a stream to Read for the first time. Data contains the first. dataSize <= m_maxHeaderSize.
            bool ReadHeaderAndData(CompressorStream* stream, AZ::u8* data, unsigned int dataSize) override;
            /// Called when we are about to start writing to a compressed stream.
            bool WriteHeaderAndData(CompressorStream* stream) override;
            /// Forwarded function from the Device when we from a compressed stream.
            SizeType Read(CompressorStream* stream, SizeType byteSize, SizeType offset, void* buffer) override;
            /// Forwarded function from the Device when we write to a compressed stream.
            SizeType Write(CompressorStream* stream, SizeType byteSize, const void* data, SizeType offset = SizeType(-1)) override;
            /// Write a seek point.
            bool WriteSeekPoint(CompressorStream* stream) override;
            /// Set auto seek point even dataSize bytes.
            bool StartCompressor(CompressorStream* stream, int compressionLevel, SizeType autoSeekDataSize) override;
            /// Called just before we close the stream. All compression data will be flushed and finalized. (You can't add data afterwards).
            bool Close(CompressorStream* stream) override;

        protected:

            /// Read as much data as possible and adjust the parameters.
            SizeType            FillFromDecompressCache(CompressorZStdData* zlibData, void*& buffer, SizeType& byteSize, SizeType& offset);
            /// Read data from stream into the compression buffer.
            SizeType            FillCompressedBuffer(CompressorStream* stream);
            /// Acquire data buffer resource (if not created it will be allocated) and increment m_dataBufferUseCount.
            void                AcquireDataBuffer();
            /// Release data buffer resource, if m_dataBufferUseCount == 0 all memory will be freed.
            void                ReleaseDataBuffer();

            CompressorStream*   m_lastReadStream = nullptr;                   ///< Cached last stream we read data into the m_dataBuffer.
            SizeType            m_lastReadStreamOffset = 0;                   ///< Offset of the last read (in the m_dataBuffer) in the compressed stream.
            SizeType            m_lastReadStreamSize = 0;                     ///< Size of the data in m_dataBuffer of the last read.

            static const int    m_CompressedDataBufferAlignment = 16;         /// Alignment for data buffer.
            unsigned char*      m_compressedDataBuffer = nullptr;             ///< Data buffer used to read/write compressed data.
            unsigned int        m_compressedDataBufferSize;                   ///< Data buffer size (stored so we can lazy allocate m_dataBuffer as we need).
            unsigned int        m_compressedDataBufferUseCount = 0;           ///< Data buffer use count.
            unsigned int        m_decompressionCachePerStream;      ///< Cache per stream for each compressed stream stream in bytes.
        };
    }   // namespace IO
}   // namespace AZ

#endif  // AZCORE_COMPRESSOR_ZSTD_H
#pragma once
