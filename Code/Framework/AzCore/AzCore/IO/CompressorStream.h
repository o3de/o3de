/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace IO
    {
        class Compressor;
        class CompressorData;
        enum class OpenMode : AZ::u32;

        /*!
         * CompressorStream wrap a GenericStream and runs the streaming functions through the supplied compressor
         *
         */
        class CompressorStream
            : public GenericStream
        {
        public:
            AZ_CLASS_ALLOCATOR(CompressorStream, SystemAllocator);
            CompressorStream(const char* filename, OpenMode flags = OpenMode());
            CompressorStream(GenericStream* stream, bool ownStream);
            virtual ~CompressorStream();

            bool IsOpen() const override;
            bool CanSeek() const override { return true; }
            bool CanRead() const override;
            bool CanWrite() const override;
            void Seek(OffsetType bytes, SeekMode mode) override;
            SizeType Read(SizeType bytes, void* oBuffer) override;
            SizeType Write(SizeType bytes, const void* iBuffer) override;
            SizeType GetCurPos() const override;

            /*!
            \brief Retrieves the length of the stream(which is the compressed length)
            \return length of the stream
            */
            SizeType GetLength() const override ///< Calls GetCompressedLength underneath the hood
            {
                return GetCompressedLength();
            }

            SizeType ReadAtOffset(SizeType bytes, void *oBuffer, OffsetType offset) override;
            SizeType WriteAtOffset(SizeType bytes, const void *iBuffer, OffsetType offset) override;
            bool IsCompressed() const override { return m_compressorData != nullptr; }
            const char* GetFilename() const override;
            OpenMode GetModeFlags() const override;
            bool ReOpen() override;
            void Close() override;

            GenericStream* GetWrappedStream() const;
            bool WriteCompressedHeader(AZ::u32 compressorId, int compressionLevel = 10, SizeType autoSeekDataSize = 0);
            bool WriteCompressedSeekPoint();
            SizeType GetCompressedLength() const; ///< Retrieves the length of the stream, which corresponds to the compressed length
            SizeType GetUncompressedLength() const; ///< Retrieves the length of the uncompressed data from the CompressorData structure
            
            void SetCompressorData(CompressorData* compressorData);
            CompressorData* GetCompressorData() const;

        protected:
            bool ReadCompressedHeader();
            Compressor* CreateCompressor(AZ::u32 compressorId);

        protected:
            GenericStream* m_stream; ///< Underlying stream to use for reading and writing raw data
            bool m_isStreamOwner; ///< Boolean which determines whether this class is responsible for ownership of the stream
            AZStd::unique_ptr<CompressorData> m_compressorData; ///< CompressorData structure used for containing metadata related to the compressor in use
            AZStd::unique_ptr<Compressor> m_compressor; ///< Compressor object responsible for performing compressions/decompression
        };

    } // namespace IO
}   // namespace AZ

