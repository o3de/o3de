/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZCORE_EXCLUDE_ZLIB)

#include <AzCore/IO/CompressorZLib.h>
#include <AzCore/IO/CompressorStream.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ::IO
{
    //=========================================================================
    // CompressorZLib
    // [12/13/2012]
    //=========================================================================
    CompressorZLib::CompressorZLib(unsigned int decompressionCachePerStream, unsigned int dataBufferSize)
        : m_lastReadStream(nullptr)
        , m_lastReadStreamOffset(0)
        , m_lastReadStreamSize(0)
        , m_compressedDataBuffer(nullptr)
        , m_compressedDataBufferSize(dataBufferSize)
        , m_compressedDataBufferUseCount(0)
        , m_decompressionCachePerStream(decompressionCachePerStream)

    {
        AZ_Assert((dataBufferSize % (32 * 1024)) == 0, "Data buffer size %d must be multiple of 32 KB!", dataBufferSize);
        AZ_Assert((decompressionCachePerStream % (32 * 1024)) == 0, "Decompress cache size %d must be multiple of 32 KB!", decompressionCachePerStream);
    }

    //=========================================================================
    // !CompressorZLib
    // [12/13/2012]
    //=========================================================================
    CompressorZLib::~CompressorZLib()
    {
        AZ_Warning("IO", m_compressedDataBufferUseCount == 0, "CompressorZLib has it's data buffer still referenced, it means that %d compressed streams have NOT closed! Freeing data...", m_compressedDataBufferUseCount);
        while (m_compressedDataBufferUseCount)
        {
            ReleaseDataBuffer();
        }
    }

    //=========================================================================
    // GetTypeId
    // [12/13/2012]
    //=========================================================================
    AZ::u32     CompressorZLib::TypeId()
    {
        return AZ_CRC_CE("ZLib");
    }

    //=========================================================================
    // ReadHeaderAndData
    // [12/13/2012]
    //=========================================================================
    bool        CompressorZLib::ReadHeaderAndData(CompressorStream* stream, AZ::u8* data, unsigned int dataSize)
    {
        if (stream->GetCompressorData() != nullptr)  // we already have compressor data
        {
            return false;
        }

        // Read the ZLib header should be after the default compression header...
        // We should not be in this function otherwise.
        if (dataSize < sizeof(CompressorZLibHeader) + sizeof(ZLib::Header))
        {
            AZ_Assert(false, "We did not read enough data, we have only %d bytes left in the buffer and we need %d!", dataSize, sizeof(CompressorZLibHeader) + sizeof(ZLib::Header));
            return false;
        }

        AcquireDataBuffer();

        CompressorZLibHeader* hdr = reinterpret_cast<CompressorZLibHeader*>(data);
        AZStd::endian_swap(hdr->m_numSeekPoints);
        dataSize -= sizeof(CompressorZLibHeader);
        data += sizeof(CompressorZLibHeader);

        CompressorZLibData* zlibData = aznew CompressorZLibData;
        zlibData->m_compressor = this;
        zlibData->m_uncompressedSize = 0;
        zlibData->m_zlibHeader = *reinterpret_cast<ZLib::Header*>(data);
        dataSize -= sizeof(zlibData->m_zlibHeader);
        data += sizeof(zlibData->m_zlibHeader);
        zlibData->m_decompressNextOffset = sizeof(CompressorHeader) + sizeof(CompressorZLibHeader) + sizeof(ZLib::Header); // start after the headers

        AZ_Assert(hdr->m_numSeekPoints > 0, "We should have at least one seek point for the entire stream!");

        // go the end of the file and read all sync points.
        SizeType compressedFileEnd = stream->GetLength();
        if (compressedFileEnd == 0)
        {
            delete zlibData;
            return false;
        }

        zlibData->m_seekPoints.resize(hdr->m_numSeekPoints);
        SizeType dataToRead = sizeof(CompressorZLibSeekPoint) * static_cast<SizeType>(hdr->m_numSeekPoints);
        SizeType seekPointOffset = compressedFileEnd - dataToRead;
        AZ_Assert(seekPointOffset <= compressedFileEnd, "We have an invalid archive, this is impossible!");
        GenericStream* baseStream = stream->GetWrappedStream();
        if (baseStream->ReadAtOffset(dataToRead, zlibData->m_seekPoints.data(), seekPointOffset) != dataToRead)
        {
            delete zlibData;
            return false;
        }
        for (size_t i = 0; i < zlibData->m_seekPoints.size(); ++i)
        {
            AZStd::endian_swap(zlibData->m_seekPoints[i].m_compressedOffset);
            AZStd::endian_swap(zlibData->m_seekPoints[i].m_uncompressedOffset);
        }

        if (m_decompressionCachePerStream)
        {
            zlibData->m_decompressedCache = reinterpret_cast<unsigned char*>(azmalloc(m_decompressionCachePerStream, m_CompressedDataBufferAlignment, AZ::SystemAllocator));
        }

        zlibData->m_decompressLastOffset = seekPointOffset; // set the start address of the seek points as the last valid read address for the compressed stream.

        zlibData->m_zlib.StartDecompressor(&zlibData->m_zlibHeader);

        stream->SetCompressorData(zlibData);

        return true;
    }

    //=========================================================================
    // WriteHeaderAndData
    // [12/13/2012]
    //=========================================================================
    bool CompressorZLib::WriteHeaderAndData(CompressorStream* stream)
    {
        if (!Compressor::WriteHeaderAndData(stream))
        {
            return false;
        }

        CompressorZLibData* compressorData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
        CompressorZLibHeader header;
        header.m_numSeekPoints = static_cast<AZ::u32>(compressorData->m_seekPoints.size());
        AZStd::endian_swap(header.m_numSeekPoints);
        GenericStream* baseStream = stream->GetWrappedStream();
        if (baseStream->WriteAtOffset(sizeof(header), &header, sizeof(CompressorHeader)) == sizeof(header))
        {
            return true;
        }

        return false;
    }

    //=========================================================================
    // FillFromDecompressCache
    // [12/14/2012]
    //=========================================================================
    inline CompressorZLib::SizeType CompressorZLib::FillFromDecompressCache(CompressorZLibData* zlibData, void*& buffer, SizeType& byteSize, SizeType& offset)
    {
        SizeType firstOffsetInCache = zlibData->m_decompressedCacheOffset;
        SizeType lastOffsetInCache = firstOffsetInCache + zlibData->m_decompressedCacheDataSize;
        SizeType firstDataOffset = offset;
        SizeType lastDataOffset = offset + byteSize;
        SizeType numCopied = 0;
        if (firstOffsetInCache < lastDataOffset && lastOffsetInCache > firstDataOffset)  // check if there is data in the cache
        {
            size_t copyOffsetStart = 0;
            size_t copyOffsetEnd = zlibData->m_decompressedCacheDataSize;

            size_t bufferCopyOffset = 0;

            if (firstOffsetInCache < firstDataOffset)
            {
                copyOffsetStart = static_cast<size_t>(firstDataOffset - firstOffsetInCache);
            }
            else
            {
                bufferCopyOffset = static_cast<size_t>(firstOffsetInCache - firstDataOffset);
            }

            if (lastOffsetInCache >= lastDataOffset)
            {
                copyOffsetEnd -= static_cast<size_t>(lastOffsetInCache - lastDataOffset);
            }
            else if (bufferCopyOffset > 0)  // the cache block is in the middle of the data, we can't use it (since we need to split buffer request into 2)
            {
                return 0;
            }

            numCopied = copyOffsetEnd - copyOffsetStart;
            memcpy(static_cast<char*>(buffer) + bufferCopyOffset, zlibData->m_decompressedCache + copyOffsetStart, static_cast<size_t>(numCopied));

            // adjust pointers and sizes
            byteSize -= numCopied;
            if (bufferCopyOffset == 0)
            {
                // copied in the start
                buffer = reinterpret_cast<char*>(buffer) + numCopied;
                offset += numCopied;
            }
        }

        return numCopied;
    }

    //=========================================================================
    // FillFromCompressedCache
    // [12/17/2012]
    //=========================================================================
    inline CompressorZLib::SizeType CompressorZLib::FillCompressedBuffer(CompressorStream* stream)
    {
        CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
        SizeType dataFromBuffer = 0;
        if (stream == m_lastReadStream)  // if the buffer is filled with data from the current stream, try to reuse
        {
            if (zlibData->m_decompressNextOffset > m_lastReadStreamOffset)
            {
                SizeType offsetInCache = zlibData->m_decompressNextOffset - m_lastReadStreamOffset;
                if (offsetInCache < m_lastReadStreamSize)  // last check if there is data overlap
                {
                    // copy the usable part at the start of the
                    SizeType toMove = m_lastReadStreamSize - offsetInCache;
                    memmove(m_compressedDataBuffer, &m_compressedDataBuffer[static_cast<size_t>(offsetInCache)], static_cast<size_t>(toMove));
                    dataFromBuffer += toMove;
                }
            }
        }

        SizeType toReadFromStream = m_compressedDataBufferSize - dataFromBuffer;
        SizeType readOffset = zlibData->m_decompressNextOffset + dataFromBuffer;
        if (readOffset + toReadFromStream > zlibData->m_decompressLastOffset)
        {
            // don't read pass the end
            AZ_Assert(readOffset <= zlibData->m_decompressLastOffset, "Read offset should always be before the end of stream!");
            toReadFromStream = zlibData->m_decompressLastOffset - readOffset;
        }

        SizeType numReadFromStream = 0;
        if (toReadFromStream)  // if we did not reuse the whole buffer, read some data from the stream
        {
            GenericStream* baseStream = stream->GetWrappedStream();
            numReadFromStream = baseStream->ReadAtOffset(toReadFromStream, &m_compressedDataBuffer[static_cast<size_t>(dataFromBuffer)], readOffset);
        }

        // update what's actually in the read data buffer.
        m_lastReadStream = stream;
        m_lastReadStreamOffset = zlibData->m_decompressNextOffset;
        m_lastReadStreamSize = dataFromBuffer + numReadFromStream;
        return m_lastReadStreamSize;
    }

    /**
     * Helper class to find the best seek point for a specific offset.
     */
    struct CompareUpper
    {
        inline bool operator()(const AZ::u64& offset, const CompressorZLibSeekPoint& sp) const {return offset < sp.m_uncompressedOffset; }
    };

    //=========================================================================
    // Read
    // [12/13/2012]
    //=========================================================================
    CompressorZLib::SizeType    CompressorZLib::Read(CompressorStream* stream, SizeType byteSize, SizeType offset, void* buffer)
    {
        AZ_Assert(stream->GetCompressorData(), "This stream doesn't have decompression enabled!");
        CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
        AZ_Assert(!zlibData->m_zlib.IsCompressorStarted(), "You can't read/decompress while writing a compressed stream %s!");

        // check if the request can be finished from the decompressed cache
        SizeType numRead = FillFromDecompressCache(zlibData, buffer, byteSize, offset);
        if (byteSize == 0)  // are we done
        {
            return numRead;
        }

        // find the best seek point for current offset
        CompressorZLibData::SeekPointArray::iterator it = AZStd::upper_bound(zlibData->m_seekPoints.begin(), zlibData->m_seekPoints.end(), offset, CompareUpper());
        AZ_Assert(it != zlibData->m_seekPoints.begin(), "This should be impossible, we should always have a valid seek point at 0 offset!");
        const CompressorZLibSeekPoint& bestSeekPoint = *(--it);  // get the previous (so it includes the current offset)

        // if read is continuous continue with decompression
        bool isJumpToSeekPoint = false;
        SizeType lastOffsetInCache = zlibData->m_decompressedCacheOffset + zlibData->m_decompressedCacheDataSize;
        if (bestSeekPoint.m_uncompressedOffset > lastOffsetInCache)     // if the best seek point is forward, jump forward to it.
        {
            isJumpToSeekPoint = true;
        }
        else if (offset < lastOffsetInCache)    // if the seek point is in the past and the requested offset is not in the cache jump back to it.
        {
            isJumpToSeekPoint = true;
        }

        if (isJumpToSeekPoint)
        {
            zlibData->m_decompressNextOffset = bestSeekPoint.m_compressedOffset;  // set next read point
            zlibData->m_decompressedCacheOffset = bestSeekPoint.m_uncompressedOffset; // set uncompressed offset
            zlibData->m_decompressedCacheDataSize = 0; // invalidate the cache
            zlibData->m_zlib.ResetDecompressor(&zlibData->m_zlibHeader); // reset decompressor and setup the header.
        }

        // decompress and move forward until the request is done
        while (byteSize > 0)
        {
            // fill buffer with compressed data
            SizeType compressedDataSize = FillCompressedBuffer(stream);
            if (compressedDataSize == 0)
            {
                return numRead; // we are done reading and obviously we did not managed to read all data
            }
            unsigned int processedCompressedData = 0;
            while (byteSize > 0 && processedCompressedData < compressedDataSize) // decompressed data either until we are done with the request (byteSize == 0) or we need to fill the compression buffer again.
            {
                // if we have data in the cache move to the next offset, we always move forward by default.
                zlibData->m_decompressedCacheOffset += zlibData->m_decompressedCacheDataSize;

                // decompress in the cache buffer
                u32 availDecompressedCacheSize = m_decompressionCachePerStream; // reset buffer size
                unsigned int processed = zlibData->m_zlib.Decompress(&m_compressedDataBuffer[processedCompressedData], static_cast<unsigned int>(compressedDataSize) - processedCompressedData, zlibData->m_decompressedCache, availDecompressedCacheSize);
                zlibData->m_decompressedCacheDataSize = m_decompressionCachePerStream - availDecompressedCacheSize;
                if (processed == 0)
                {
                    break; // we processed everything we could, load more compressed data.
                }
                processedCompressedData += processed;
                // fill what we can from the cache
                numRead += FillFromDecompressCache(zlibData, buffer, byteSize, offset);
            }
            // update next read position the the compressed stream
            zlibData->m_decompressNextOffset +=  processedCompressedData;
        }
        return numRead;
    }

    //=========================================================================
    // Write
    // [12/13/2012]
    //=========================================================================
    CompressorZLib::SizeType    CompressorZLib::Write(CompressorStream* stream, SizeType byteSize, const void* data, SizeType offset)
    {
        (void)offset;

        AZ_Assert(stream && stream->GetCompressorData(), "This stream doesn't have compression enabled! Call Stream::WriteCompressed after you create the file!");
        AZ_Assert(offset == SizeType(-1) || offset == stream->GetCurPos(), "We can write compressed data only at the end of the stream!");

        m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

        CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
        AZ_Assert(!zlibData->m_zlib.IsDecompressorStarted(), "You can't write while reading/decompressing a compressed stream!");

        const u8* bytes = reinterpret_cast<const u8*>(data);
        unsigned int dataToCompress = static_cast<unsigned int>(byteSize);
        while (dataToCompress != 0)
        {
            unsigned int oldDataToCompress = dataToCompress;
            unsigned int compressedSize = zlibData->m_zlib.Compress(bytes, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize);
            if (compressedSize)
            {
                GenericStream* baseStream = stream->GetWrappedStream();
                SizeType numWritten = baseStream->Write(compressedSize, m_compressedDataBuffer);
                if (numWritten != compressedSize)
                {
                    return numWritten; // error we could not write all data
                }
            }
            bytes += oldDataToCompress - dataToCompress;
        }
        zlibData->m_uncompressedSize += byteSize;

        if (zlibData->m_autoSeekSize > 0)
        {
            // insert a seek point if needed.
            if (zlibData->m_seekPoints.empty())
            {
                if (zlibData->m_uncompressedSize >=  zlibData->m_autoSeekSize)
                {
                    WriteSeekPoint(stream);
                }
            }
            else if ((zlibData->m_uncompressedSize - zlibData->m_seekPoints.back().m_uncompressedOffset) > zlibData->m_autoSeekSize)
            {
                WriteSeekPoint(stream);
            }
        }
        return byteSize;
    }

    //=========================================================================
    // WriteSeekPoint
    // [12/13/2012]
    //=========================================================================
    bool        CompressorZLib::WriteSeekPoint(CompressorStream* stream)
    {
        AZ_Assert(stream && stream->GetCompressorData(), "This stream doesn't have compression enabled! Call Stream::WriteCompressed after you create the file!");
        CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());

        m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

        unsigned int compressedSize;
        unsigned int dataToCompress = 0;
        do
        {
            compressedSize = zlibData->m_zlib.Compress(nullptr, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize, ZLib::FT_FULL_FLUSH);
            if (compressedSize)
            {
                GenericStream* baseStream = stream->GetWrappedStream();
                SizeType numWritten = baseStream->Write(compressedSize, m_compressedDataBuffer);
                if (numWritten != compressedSize)
                {
                    return false; // error we wrote less than than requested!
                }
            }
        } while (dataToCompress != 0);

        CompressorZLibSeekPoint sp;
        sp.m_compressedOffset = stream->GetLength();
        sp.m_uncompressedOffset = zlibData->m_uncompressedSize;
        zlibData->m_seekPoints.push_back(sp);
        return true;
    }

    //=========================================================================
    // StartCompressor
    // [12/13/2012]
    //=========================================================================
    bool        CompressorZLib::StartCompressor(CompressorStream* stream, int compressionLevel, SizeType autoSeekDataSize)
    {
        AZ_Assert(stream && stream->GetCompressorData() == nullptr, "Stream has compressor already enabled!");

        AcquireDataBuffer();

        CompressorZLibData* zlibData = aznew CompressorZLibData;
        zlibData->m_compressor = this;
        zlibData->m_zlibHeader = 0; // not used for compression
        zlibData->m_uncompressedSize = 0;
        zlibData->m_autoSeekSize = autoSeekDataSize;
        compressionLevel = AZ::GetClamp(compressionLevel, 1, 9); // remap to zlib levels

        zlibData->m_zlib.StartCompressor(compressionLevel);

        stream->SetCompressorData(zlibData);

        if (WriteHeaderAndData(stream))
        {
            // add the first and always present seek point at the start of the compressed stream
            CompressorZLibSeekPoint sp;
            sp.m_compressedOffset = sizeof(CompressorHeader) + sizeof(CompressorZLibHeader) + sizeof(zlibData->m_zlibHeader);
            sp.m_uncompressedOffset = 0;
            zlibData->m_seekPoints.push_back(sp);
            return true;
        }
        return false;
    }

    //=========================================================================
    // Close
    // [12/13/2012]
    //=========================================================================
    bool CompressorZLib::Close(CompressorStream* stream)
    {
        AZ_Assert(stream->IsOpen(), "Stream is not open to be closed!");

        CompressorZLibData* zlibData = static_cast<CompressorZLibData*>(stream->GetCompressorData());
        GenericStream* baseStream = stream->GetWrappedStream();

        bool result = true;
        if (zlibData->m_zlib.IsCompressorStarted())
        {
            m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

            // flush all compressed data
            unsigned int compressedSize;
            unsigned int dataToCompress = 0;
            do
            {
                compressedSize = zlibData->m_zlib.Compress(nullptr, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize, ZLib::FT_FINISH);
                if (compressedSize)
                {
                    baseStream->Write(compressedSize, m_compressedDataBuffer);
                }
            } while (dataToCompress != 0);

            result = WriteHeaderAndData(stream);
            if (result)
            {
                // now write the seek points and the end of the file
                for (size_t i = 0; i < zlibData->m_seekPoints.size(); ++i)
                {
                    AZStd::endian_swap(zlibData->m_seekPoints[i].m_compressedOffset);
                    AZStd::endian_swap(zlibData->m_seekPoints[i].m_uncompressedOffset);
                }
                SizeType dataToWrite = zlibData->m_seekPoints.size() * sizeof(CompressorZLibSeekPoint);
                baseStream->Seek(0U, GenericStream::SeekMode::ST_SEEK_END);
                result = (baseStream->Write(dataToWrite, zlibData->m_seekPoints.data()) == dataToWrite);
            }
        }
        else
        {
            if (m_lastReadStream == stream)
            {
                m_lastReadStream = nullptr; // invalidate the data in m_dataBuffer if it was from the current stream.
            }
        }

        // if we have decompressor cache delete it
        if (zlibData->m_decompressedCache)
        {
            azfree(zlibData->m_decompressedCache, AZ::SystemAllocator, m_decompressionCachePerStream, m_CompressedDataBufferAlignment);
        }

        ReleaseDataBuffer();

        // last step reset strream compressor data.
        stream->SetCompressorData(nullptr);
        return result;
    }

    //=========================================================================
    // AcquireDataBuffer
    // [2/27/2013]
    //=========================================================================
    void CompressorZLib::AcquireDataBuffer()
    {
        if (m_compressedDataBuffer == nullptr)
        {
            AZ_Assert(m_compressedDataBufferUseCount == 0, "Buffer usecount should be 0 if the buffer is NULL");
            m_compressedDataBuffer = reinterpret_cast<unsigned char*>(azmalloc(m_compressedDataBufferSize, m_CompressedDataBufferAlignment, AZ::SystemAllocator));
            m_lastReadStream = nullptr; // reset the cache info in the m_dataBuffer
        }
        ++m_compressedDataBufferUseCount;
    }

    //=========================================================================
    // ReleaseDataBuffer
    // [2/27/2013]
    //=========================================================================
    void CompressorZLib::ReleaseDataBuffer()
    {
        --m_compressedDataBufferUseCount;
        if (m_compressedDataBufferUseCount == 0)
        {
            AZ_Assert(m_compressedDataBuffer != nullptr, "Invalid data buffer! We should have a non null pointer!");
            azfree(m_compressedDataBuffer, AZ::SystemAllocator, m_compressedDataBufferSize, m_CompressedDataBufferAlignment);
            m_compressedDataBuffer = nullptr;
            m_lastReadStream = nullptr; // reset the cache info in the m_dataBuffer
        }
    }
} // namespace AZ::IO

#endif // #if !defined(AZCORE_EXCLUDE_ZLIB)
