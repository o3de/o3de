/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

namespace AZ::IO
{
    class CompressorStream;

    /**
     * Compressor/Decompressor base interface.
     * Used for all stream compressors.
     */
    class Compressor
    {
    public:
        typedef AZ::u64 SizeType;
        static const int m_maxHeaderSize = 4096; /// When we open a stream to check if it's compressed we read the first m_maxHeaderSize bytes.

        virtual ~Compressor() {}
        /// Return compressor type id.
        virtual AZ::u32     GetTypeId() const = 0;
        /// Called when we open a stream to Read for the first time. Data contains the first. dataSize <= m_maxHeaderSize.
        virtual bool        ReadHeaderAndData(CompressorStream* stream, AZ::u8* data, unsigned int dataSize) = 0;
        /// Called when we are about to start writing to a compressed stream. (Must be called first to write compressor header)
        virtual bool        WriteHeaderAndData(CompressorStream* stream);
        /// Forwarded function from the Device when we from a compressed stream.
        virtual SizeType    Read(CompressorStream* stream, SizeType byteSize, SizeType offset, void* buffer) = 0;
        /// Forwarded function from the Device when we write to a compressed stream.
        virtual SizeType    Write(CompressorStream* stream, SizeType byteSize, const void* data, SizeType offset = SizeType(-1)) = 0;
        /// Write a seek point.
        virtual bool        WriteSeekPoint(CompressorStream* stream)                                                          { (void)stream; return false; }
        /// Initializes Compressor for writing data.
        virtual bool        StartCompressor(CompressorStream* stream, int compressionLevel, SizeType autoSeekDataSize)        { (void)stream; (void)compressionLevel; (void)autoSeekDataSize; return false; }
        /// Called just before we close the stream. All compression data will be flushed and finalized. (You can't add data afterwards).
        virtual bool        Close(CompressorStream* stream) = 0;
    };

    /**
     * Base compressor data assigned for all compressors.
     */
    class CompressorData
    {
    public:
        virtual ~CompressorData() {}

        Compressor*     m_compressor;
        AZ::u64         m_uncompressedSize;
    };

    /**
     * All data is stored in network order (big endian).
     */
    struct  CompressorHeader
    {
        CompressorHeader()      { m_azcs[0] = 0; m_azcs[1] = 0; m_azcs[2] = 0; m_azcs[3] = 0; }

        bool IsValid() const    { return (m_azcs[0] == 'A' && m_azcs[1] == 'Z' && m_azcs[2] == 'C' && m_azcs[3] == 'S'); }
        void SetAZCS()          { m_azcs[0] = 'A'; m_azcs[1] = 'Z'; m_azcs[2] = 'C'; m_azcs[3] = 'S'; }

        char        m_azcs[4];          ///< String contains 'AZCS' AmaZon Compressed Stream
        AZ::u32     m_compressorId;     ///< Compression method.
        AZ::u64     m_uncompressedSize; ///< Uncompressed file size.
    };
} // namespace AZ::IO
