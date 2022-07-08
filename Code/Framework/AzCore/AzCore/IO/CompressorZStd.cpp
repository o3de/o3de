/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZCORE_EXCLUDE_ZSTD)

#include <AzCore/IO/CompressorZStd.h>
#include <AzCore/IO/CompressorStream.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ::IO
{
    CompressorZStd::CompressorZStd(unsigned int decompressionCachePerStream, unsigned int dataBufferSize)
        : m_compressedDataBufferSize(dataBufferSize)
        , m_decompressionCachePerStream(decompressionCachePerStream)

    {
        AZ_Assert((dataBufferSize % (32 * 1024)) == 0, "Data buffer size %d must be multiple of 32 KB.", dataBufferSize);
        AZ_Assert((decompressionCachePerStream % (32 * 1024)) == 0, "Decompress cache size %d must be multiple of 32 KB.", decompressionCachePerStream);
    }

    CompressorZStd::~CompressorZStd()
    {
        AZ_Warning("IO", m_compressedDataBufferUseCount == 0, "CompressorZStd has it's data buffer still referenced, it means that %d compressed streams have NOT closed. Freeing data...", m_compressedDataBufferUseCount);
        while (m_compressedDataBufferUseCount)
        {
            ReleaseDataBuffer();
        }
    }

    AZ::u32 CompressorZStd::TypeId()
    {
        return AZ_CRC("ZStd", 0x72fd505e);
    }

    bool CompressorZStd::ReadHeaderAndData(CompressorStream* stream, AZ::u8* data, unsigned int dataSize)
    {
        if (stream->GetCompressorData() != nullptr)  // we already have compressor data
        {
            return false;
        }

        // Read the ZStd header should be after the default compression header...
        // We should not be in this function otherwise.
        if (dataSize < sizeof(CompressorZStdHeader) + sizeof(ZStd::Header))
        {
            AZ_Error("CompressorZStd", false, "We did not read enough data, we have only %d bytes left in the buffer and we need %d.", dataSize, sizeof(CompressorZStdHeader) + sizeof(ZStd::Header));
            return false;
        }

        AcquireDataBuffer();

        CompressorZStdHeader* hdr = reinterpret_cast<CompressorZStdHeader*>(data);
        dataSize -= sizeof(CompressorZStdHeader);
        data += sizeof(CompressorZStdHeader);

        AZStd::unique_ptr<CompressorZStdData> zstdData = AZStd::make_unique<CompressorZStdData>();
        zstdData->m_compressor = this;
        zstdData->m_uncompressedSize = 0;
        zstdData->m_zstdHeader = *reinterpret_cast<ZStd::Header*>(data);
        dataSize -= sizeof(zstdData->m_zstdHeader);
        data += sizeof(zstdData->m_zstdHeader);
        zstdData->m_decompressNextOffset = sizeof(CompressorHeader) + sizeof(CompressorZStdHeader) + sizeof(ZStd::Header); // start after the headers

        AZ_Error("CompressorZStd", hdr->m_numSeekPoints > 0, "We should have at least one seek point for the entire stream.");

        // go the end of the file and read all sync points.
        SizeType compressedFileEnd = stream->GetLength();
        if (compressedFileEnd == 0)
        {
            return false;
        }

        zstdData->m_seekPoints.resize(hdr->m_numSeekPoints);
        SizeType dataToRead = sizeof(CompressorZStdSeekPoint) * static_cast<SizeType>(hdr->m_numSeekPoints);
        SizeType seekPointOffset = compressedFileEnd - dataToRead;

        if (seekPointOffset > compressedFileEnd)
        {
            AZ_Error("CompressorZStd", false, "We have an invalid archive, this is impossible.");
            return false;
        }

        GenericStream* baseStream = stream->GetWrappedStream();
        if (baseStream->ReadAtOffset(dataToRead, zstdData->m_seekPoints.data(), seekPointOffset) != dataToRead)
        {
            return false;
        }

        if (m_decompressionCachePerStream)
        {
            zstdData->m_decompressedCache = reinterpret_cast<unsigned char*>(azmalloc(m_decompressionCachePerStream, m_CompressedDataBufferAlignment, AZ::SystemAllocator, "CompressorZStd"));
        }

        zstdData->m_decompressLastOffset = seekPointOffset; // set the start address of the seek points as the last valid read address for the compressed stream.

        zstdData->m_zstd.StartDecompressor();

        stream->SetCompressorData(zstdData.release());

        return true;
    }

    bool CompressorZStd::WriteHeaderAndData(CompressorStream* stream)
    {
        if (!Compressor::WriteHeaderAndData(stream))
        {
            return false;
        }

        CompressorZStdData* compressorData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
        CompressorZStdHeader header;
        header.m_numSeekPoints = aznumeric_caster(compressorData->m_seekPoints.size());
        GenericStream* baseStream = stream->GetWrappedStream();
        if (baseStream->WriteAtOffset(sizeof(header), &header, sizeof(CompressorHeader)) == sizeof(header))
        {
            return true;
        }

        return false;
    }

    inline CompressorZStd::SizeType CompressorZStd::FillFromDecompressCache(CompressorZStdData* zstdData, void*& buffer, SizeType& byteSize, SizeType& offset)
    {
        SizeType firstOffsetInCache = zstdData->m_decompressedCacheOffset;
        SizeType lastOffsetInCache = firstOffsetInCache + zstdData->m_decompressedCacheDataSize;
        SizeType firstDataOffset = offset;
        SizeType lastDataOffset = offset + byteSize;
        SizeType numCopied = 0;
        if (firstOffsetInCache < lastDataOffset && lastOffsetInCache >= firstDataOffset)  // check if there is data in the cache
        {
            size_t copyOffsetStart = 0;
            size_t copyOffsetEnd = zstdData->m_decompressedCacheDataSize;

            size_t bufferCopyOffset = 0;

            if (firstOffsetInCache < firstDataOffset)
            {
                copyOffsetStart = aznumeric_caster(firstDataOffset - firstOffsetInCache);
            }
            else
            {
                bufferCopyOffset = aznumeric_caster(firstOffsetInCache - firstDataOffset);
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
            memcpy(static_cast<char*>(buffer) + bufferCopyOffset, zstdData->m_decompressedCache + copyOffsetStart, static_cast<size_t>(numCopied));

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

    inline CompressorZStd::SizeType CompressorZStd::FillCompressedBuffer(CompressorStream* stream)
    {
        CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
        SizeType dataFromBuffer = 0;
        if (stream == m_lastReadStream)  // if the buffer is filled with data from the current stream, try to reuse
        {
            if (zstdData->m_decompressNextOffset > m_lastReadStreamOffset)
            {
                SizeType offsetInCache = zstdData->m_decompressNextOffset - m_lastReadStreamOffset;
                if (offsetInCache < m_lastReadStreamSize)  // last check if there is data overlap
                {
                    // copy the usable part at the start of the buffer
                    SizeType toMove = m_lastReadStreamSize - offsetInCache;
                    memcpy(m_compressedDataBuffer, &m_compressedDataBuffer[static_cast<size_t>(offsetInCache)], static_cast<size_t>(toMove));
                    dataFromBuffer += toMove;
                }
            }
        }

        SizeType toReadFromStream = m_compressedDataBufferSize - dataFromBuffer;
        SizeType readOffset = zstdData->m_decompressNextOffset + dataFromBuffer;
        if (readOffset + toReadFromStream > zstdData->m_decompressLastOffset)
        {
            // don't read past the end
            AZ_Assert(readOffset <= zstdData->m_decompressLastOffset, "Read offset should always be before the end of stream.");
            toReadFromStream = zstdData->m_decompressLastOffset - readOffset;
        }

        SizeType numReadFromStream = 0;
        if (toReadFromStream)  // if we did not reuse the whole buffer, read some data from the stream
        {
            GenericStream* baseStream = stream->GetWrappedStream();
            numReadFromStream = baseStream->ReadAtOffset(toReadFromStream, &m_compressedDataBuffer[static_cast<size_t>(dataFromBuffer)], readOffset);
        }

        // update what's actually in the read data buffer.
        m_lastReadStream = stream;
        m_lastReadStreamOffset = zstdData->m_decompressNextOffset;
        m_lastReadStreamSize = dataFromBuffer + numReadFromStream;
        return m_lastReadStreamSize;
    }

    struct ZStdCompareUpper
    {
        bool operator()(const AZ::u64& offset, const CompressorZStdSeekPoint& sp) const
        {
            return offset < sp.m_uncompressedOffset;
        }
    };

    CompressorZStd::SizeType CompressorZStd::Read(CompressorStream* stream, SizeType byteSize, SizeType offset, void* buffer)
    {
        AZ_Assert(stream->GetCompressorData(), "This stream doesn't have decompression enabled.");
        CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
        AZ_Assert(!zstdData->m_zstd.IsCompressorStarted(), "You can't read/decompress while writing a compressed stream %s.");

        // check if the request can be finished from the decompressed cache
        SizeType numRead = FillFromDecompressCache(zstdData, buffer, byteSize, offset);
        if (byteSize == 0)  // are we done
        {
            return numRead;
        }

        // find the best seek point for current offset
        CompressorZStdData::SeekPointArray::iterator it = AZStd::upper_bound(zstdData->m_seekPoints.begin(), zstdData->m_seekPoints.end(), offset, ZStdCompareUpper());
        AZ_Assert(it != zstdData->m_seekPoints.begin(), "This should be impossible, we should always have a valid seek point at 0 offset.");
        const CompressorZStdSeekPoint& bestSeekPoint = *(--it);  // get the previous (so it includes the current offset)

        // if read is continuous continue with decompression
        bool isJumpToSeekPoint = false;
        SizeType lastOffsetInCache = zstdData->m_decompressedCacheOffset + zstdData->m_decompressedCacheDataSize;
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
            zstdData->m_decompressNextOffset = bestSeekPoint.m_compressedOffset;  // set next read point
            zstdData->m_decompressedCacheOffset = bestSeekPoint.m_uncompressedOffset; // set uncompressed offset
            zstdData->m_decompressedCacheDataSize = 0; // invalidate the cache
            zstdData->m_zstd.ResetDecompressor(&zstdData->m_zstdHeader); // reset decompressor and setup the header.
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
                zstdData->m_decompressedCacheOffset += zstdData->m_decompressedCacheDataSize;

                // decompress in the cache buffer
                u32 availDecompressedCacheSize = m_decompressionCachePerStream; // reset buffer size
                size_t nextBlockSize;
                unsigned int processed = zstdData->m_zstd.Decompress(&m_compressedDataBuffer[processedCompressedData],
                                                                    static_cast<unsigned int>(compressedDataSize) - processedCompressedData,
                                                                    zstdData->m_decompressedCache,
                                                                    availDecompressedCacheSize,
                                                                    &nextBlockSize);
                zstdData->m_decompressedCacheDataSize = m_decompressionCachePerStream - availDecompressedCacheSize;
                if (processed == 0)
                {
                    break; // we processed everything we could, load more compressed data.
                }
                processedCompressedData += processed;
                // fill what we can from the cache
                numRead += FillFromDecompressCache(zstdData, buffer, byteSize, offset);
            }
            // update next read position the the compressed stream
            zstdData->m_decompressNextOffset +=  processedCompressedData;
        }
        return numRead;
    }

    CompressorZStd::SizeType CompressorZStd::Write(CompressorStream* stream, SizeType byteSize, const void* data, SizeType offset)
    {
        AZ_UNUSED(offset);

        AZ_Assert(stream && stream->GetCompressorData(), "This stream doesn't have compression enabled. Call Stream::WriteCompressed after you create the file.");
        AZ_Assert(offset == SizeType(-1) || offset == stream->GetCurPos(), "We can write compressed data only at the end of the stream.");

        m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

        CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
        AZ_Assert(!zstdData->m_zstd.IsDecompressorStarted(), "You can't write while reading/decompressing a compressed stream.");

        const u8* bytes = reinterpret_cast<const u8*>(data);
        unsigned int dataToCompress = aznumeric_caster(byteSize);
        while (dataToCompress != 0)
        {
            unsigned int oldDataToCompress = dataToCompress;
            unsigned int compressedSize = zstdData->m_zstd.Compress(bytes, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize);
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
        zstdData->m_uncompressedSize += byteSize;

        if (zstdData->m_autoSeekSize > 0)
        {
            // insert a seek point if needed.
            if (zstdData->m_seekPoints.empty())
            {
                if (zstdData->m_uncompressedSize >=  zstdData->m_autoSeekSize)
                {
                    WriteSeekPoint(stream);
                }
            }
            else if ((zstdData->m_uncompressedSize - zstdData->m_seekPoints.back().m_uncompressedOffset) > zstdData->m_autoSeekSize)
            {
                WriteSeekPoint(stream);
            }
        }
        return byteSize;
    }

    bool CompressorZStd::WriteSeekPoint(CompressorStream* stream)
    {
        AZ_Assert(stream && stream->GetCompressorData(), "This stream doesn't have compression enabled. Call Stream::WriteCompressed after you create the file.");
        CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());

        m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

        unsigned int compressedSize;
        unsigned int dataToCompress = 0;
        do
        {
            compressedSize = zstdData->m_zstd.Compress(nullptr, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize, ZStd::FT_FULL_FLUSH);
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

        CompressorZStdSeekPoint sp;
        sp.m_compressedOffset = stream->GetLength();
        sp.m_uncompressedOffset = zstdData->m_uncompressedSize;
        zstdData->m_seekPoints.push_back(sp);
        return true;
    }

    bool CompressorZStd::StartCompressor(CompressorStream* stream, int compressionLevel, SizeType autoSeekDataSize)
    {
        AZ_Assert(stream && !stream->GetCompressorData(), "Stream has compressor already enabled.");

        AcquireDataBuffer();

        CompressorZStdData* zstdData = aznew CompressorZStdData;
        zstdData->m_compressor = this;
        zstdData->m_zstdHeader = 0; // not used for compression
        zstdData->m_uncompressedSize = 0;
        zstdData->m_autoSeekSize = autoSeekDataSize;
        compressionLevel = AZ::GetClamp(compressionLevel, 1, 9); // remap to zlib levels

        zstdData->m_zstd.StartCompressor(compressionLevel);

        stream->SetCompressorData(zstdData);

        if (WriteHeaderAndData(stream))
        {
            // add the first and always present seek point at the start of the compressed stream
            CompressorZStdSeekPoint sp;
            sp.m_compressedOffset = sizeof(CompressorHeader) + sizeof(CompressorZStdHeader) + sizeof(zstdData->m_zstdHeader);
            sp.m_uncompressedOffset = 0;
            zstdData->m_seekPoints.push_back(sp);
            return true;
        }
        return false;
    }

    bool CompressorZStd::Close(CompressorStream* stream)
    {
        AZ_Assert(stream->IsOpen(), "Stream is not open to be closed.");

        CompressorZStdData* zstdData = static_cast<CompressorZStdData*>(stream->GetCompressorData());
        GenericStream* baseStream = stream->GetWrappedStream();

        bool result = true;
        if (zstdData->m_zstd.IsCompressorStarted())
        {
            m_lastReadStream = nullptr; // invalidate last read position, otherwise m_dataBuffer will be corrupted (as we are about to write in it).

            // flush all compressed data
            unsigned int compressedSize;
            unsigned int dataToCompress = 0;
            do
            {
                compressedSize = zstdData->m_zstd.Compress(nullptr, dataToCompress, m_compressedDataBuffer, m_compressedDataBufferSize, ZStd::FT_FINISH);
                if (compressedSize)
                {
                    baseStream->Write(compressedSize, m_compressedDataBuffer);
                }
            } while (dataToCompress != 0);

            result = WriteHeaderAndData(stream);
            if (result)
            {
                SizeType dataToWrite = zstdData->m_seekPoints.size() * sizeof(CompressorZStdSeekPoint);
                baseStream->Seek(0U, GenericStream::SeekMode::ST_SEEK_END);
                result = (baseStream->Write(dataToWrite, zstdData->m_seekPoints.data()) == dataToWrite);
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
        if (zstdData->m_decompressedCache)
        {
            azfree(zstdData->m_decompressedCache, AZ::SystemAllocator, m_decompressionCachePerStream, m_CompressedDataBufferAlignment);
        }

        ReleaseDataBuffer();

        // last step reset strream compressor data.
        stream->SetCompressorData(nullptr);
        return result;
    }

    void CompressorZStd::AcquireDataBuffer()
    {
        if (m_compressedDataBuffer == nullptr)
        {
            AZ_Assert(m_compressedDataBufferUseCount == 0, "Buffer usecount should be 0 if the buffer is NULL");
            m_compressedDataBuffer = reinterpret_cast<unsigned char*>(azmalloc(m_compressedDataBufferSize, m_CompressedDataBufferAlignment, AZ::SystemAllocator, "CompressorZStd"));
            m_lastReadStream = nullptr; // reset the cache info in the m_dataBuffer
        }
        ++m_compressedDataBufferUseCount;
    }

    void CompressorZStd::ReleaseDataBuffer()
    {
        --m_compressedDataBufferUseCount;
        if (m_compressedDataBufferUseCount == 0)
        {
            AZ_Assert(m_compressedDataBuffer, "Invalid data buffer. We should have a non null pointer.");
            azfree(m_compressedDataBuffer, AZ::SystemAllocator, m_compressedDataBufferSize, m_CompressedDataBufferAlignment);
            m_compressedDataBuffer = nullptr;
            m_lastReadStream = nullptr; // reset the cache info in the m_dataBuffer
        }
    }
} // namespace AZ::IO

#endif // #if !defined(AZCORE_EXCLUDE_ZSTD)
