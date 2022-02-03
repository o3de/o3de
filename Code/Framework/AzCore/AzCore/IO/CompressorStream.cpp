/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/IO/CompressorStream.h>
#include <AzCore/IO/Compressor.h>
#include <AzCore/IO/CompressorZLib.h>
#include <AzCore/IO/CompressorZStd.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Memory/Memory.h>

namespace AZ::IO
{
/*!
\brief Constructs a compressor stream using the supplied filename and OpenFlags to open a file on disk
\param filename name of file on disk to open. A FileIOStream is used internally.
\param flags flags used to open @filename
*/
CompressorStream::CompressorStream(const char* filename, OpenMode flags /*= 0*/)
    : CompressorStream(aznew FileIOStream(filename, flags), true)
{
}

/*!
\brief Constructs a compressor stream using the supplied stream 
\param stream generic stream object to use for reading/writing compressed data
\param ownStream if true the compressor stream takes over ownership of the supplied stream and deletes it on destruction
*/
CompressorStream::CompressorStream(GenericStream* stream, bool ownStream)
    : m_stream(stream)
    , m_isStreamOwner(ownStream)
{
    if (IsOpen())
    {
        ReadCompressedHeader();
    }
}

CompressorStream::~CompressorStream()
{
    if (m_stream->IsOpen() && m_compressorData && m_compressorData->m_compressor)
    {
        m_compressorData->m_compressor->Close(this);
    }

    if (m_isStreamOwner)
    {
        delete m_stream;
    }
}

bool CompressorStream::IsOpen() const
{
    return m_stream ? m_stream->IsOpen() : false;
}

bool CompressorStream::CanRead() const
{
    return m_stream && m_stream->CanRead();
}

bool CompressorStream::CanWrite() const
{
    return m_stream && m_stream->CanWrite();
}

void CompressorStream::Seek(OffsetType bytes, SeekMode mode)
{
    if (m_stream)
    {
        m_stream->Seek(bytes, mode);
    }
}

SizeType CompressorStream::Read(SizeType bytes, void* oBuffer)
{
    return m_compressorData && m_compressorData->m_compressor ? m_compressorData->m_compressor->Read(this, bytes, GetCurPos(), oBuffer) : m_stream->Read(bytes, oBuffer);
}

SizeType CompressorStream::Write(SizeType bytes, const void* iBuffer)
{
    return m_compressorData && m_compressorData->m_compressor ? m_compressorData->m_compressor->Write(this, bytes, iBuffer, GetCurPos()) : m_stream->Write(bytes, iBuffer);
}

AZ::IO::SizeType CompressorStream::GetCurPos() const
{
    return m_stream ? m_stream->GetCurPos() : 0;
}

/*!
\brief Reads from the compressor stream using the specified offset
\param bytes Amount of bytes to read
\param oBuffer Buffer to read bytes into. This buffer must be at least sizeof(@bytes)
\param offset If the compressor is active then this offset is passed into the compressor to determine where the nearest seek point is located otherwise the underlying stream ReadAtOffset is called.
*/
SizeType CompressorStream::ReadAtOffset(SizeType bytes, void *oBuffer, OffsetType offset)
{
    return m_compressorData && m_compressorData->m_compressor ? m_compressorData->m_compressor->Read(this, bytes, offset, oBuffer) : m_stream->ReadAtOffset(bytes, oBuffer, offset);
}

/*!
\brief Writes to the stream using the specified offset
\param bytes Amount of bytes to write
\param iBuffer Buffer containing data to write
\param offset If the compressor is active then this offset is passed into the compressor
*/
SizeType CompressorStream::WriteAtOffset(SizeType bytes, const void *iBuffer, OffsetType offset)
{
    return m_compressorData && m_compressorData->m_compressor ? m_compressorData->m_compressor->Write(this, bytes, iBuffer, offset) : m_stream->WriteAtOffset(bytes, iBuffer, offset);
}

const char* CompressorStream::GetFilename() const
{
    return m_stream->GetFilename();
}

OpenMode CompressorStream::GetModeFlags() const
{
    return m_stream->GetModeFlags();
}

bool CompressorStream::ReOpen()
{
    if (m_stream->ReOpen())
    {
        ReadCompressedHeader();
        return true;
    }
    return false;
}

void CompressorStream::Close()
{
    if (m_stream->IsOpen() && m_compressorData && m_compressorData->m_compressor)
    {
        m_compressorData->m_compressor->Close(this);
    }
    m_stream->Close();
}

/*!
\brief Retrieves the stream that this compressor wraps
*/
AZ::IO::GenericStream* CompressorStream::GetWrappedStream() const
{
    return m_stream;
}

/*!
\brief Initializes the CompressorData structure and writes compressor specific header data to the beginning of the stream
\param compressionLevel compression level used to tweak the speed of compression vs size
\param autoSeekDataSize The amount of bytes between writing of seek points and flushing the compressed data to the strream
Seeks points are used for accessing specific uncompressed offsets within a compressed file
\return true if the compressor data is able to be initialized and the compressor header is written
*/
bool CompressorStream::WriteCompressedHeader(AZ::u32 compressorId, int compressionLevel, SizeType autoSeekDataSize)
{
    if (m_compressorData)
    {
        AZ_Warning("IO", false, "Compressor already started!");
        return false;
    }
    if (GetCompressedLength() > 0)
    {
        AZ_Warning("IO", false, "We can start compressing a stream only before we have written any data to it! Currently you have %llu bytes written!", GetCompressedLength());
        return false;
    }

    if (!CreateCompressor(compressorId))
    {
        AZ_Warning("IO", false, "Compressor is non-existent, unable to write compressed header");
        return false;
    }

    return m_compressor->StartCompressor(this, compressionLevel, autoSeekDataSize);
}

/*!
\brief Check if stream open for read is compressed, and if so (returns true) and create the CompressorData structure
\return true if the stream is open for read and contains a valid compressed header 
*/
bool CompressorStream::ReadCompressedHeader()
{
    if (!CanRead())
    {
        return false;
    }

    // Read the compressed header from the underlying stream
    AZStd::array<AZ::u8, Compressor::m_maxHeaderSize> dataBuffer;
    unsigned int dataSize = static_cast<unsigned int>(m_stream->ReadAtOffset(dataBuffer.size(), dataBuffer.data(), 0U));

    if (dataSize > sizeof(CompressorHeader)) // at lea
    {
        CompressorHeader* hdr = reinterpret_cast<CompressorHeader*>(dataBuffer.data());
        if (hdr->IsValid())
        {
            AZStd::endian_swap(hdr->m_compressorId);
            AZStd::endian_swap(hdr->m_uncompressedSize);
            AZ::u8* data = dataBuffer.data() + sizeof(CompressorHeader);
            dataSize -= sizeof(CompressorHeader);

            Compressor* compressor = CreateCompressor(hdr->m_compressorId);
            AZ_Assert(compressor, "Can't create compressor 0x%08x for stream! We can't load this stream!", hdr->m_compressorId);

            if (compressor && compressor->ReadHeaderAndData(this, data, dataSize))
            {
               m_compressorData->m_uncompressedSize = hdr->m_uncompressedSize;
            }
            else
            {
                AZ_Assert(false, "Failed to setup compressor 0x%08x for stream!", hdr->m_compressorId);
            }

            return true;
        }

        return false;
    }

    return false;
}

/*!
\brief Flushes the compressed data to the stream and adds a compressed->uncompressed seek point mapping to the compressor data
\return true if the compressor exist and all the compressed data is flushed to the stream
*/
bool CompressorStream::WriteCompressedSeekPoint()
{
    return m_compressorData && m_compressorData->m_compressor ? m_compressorData->m_compressor->WriteSeekPoint(this) : true;
}

/*!
\brief Retrieves the compressed length of the stream
\return compressed length of the stream
*/
SizeType CompressorStream::GetCompressedLength() const
{
    return m_stream ? m_stream->GetLength() : 0;
}

/*!
\brief Retrieves the uncompressed length of the stream if compressor data is associated with the stream
\return uncompressed length of stream unless compressor data is non-existent on the stream. Otherwise GetCompressedLength is called
The compressor data is non-existent if the stream is open for read and there is no header data or if the stream is open for write and WriteCompressedHeader has not been called
*/
SizeType CompressorStream::GetUncompressedLength() const
{
    return m_compressorData ? m_compressorData->m_uncompressedSize : GetCompressedLength();
}

/*!
\brief Assigns supplied compressor data object to this stream
\param compressorData compressor data object to assign 
*/
void CompressorStream::SetCompressorData(CompressorData* compressorData)
{
    m_compressorData.reset(compressorData);
}

/*!
\brief Retrieves the compressor data object associated with this stream
\return compressor data assigned to this stream
*/
CompressorData* CompressorStream::GetCompressorData() const
{
    return m_compressorData.get();
}

/*!
\brief Creates a compressor which has a type id of @compressorId and return the compressor
\detail The created compressor will be owned by this stream
\param compressorId Type id of compressor to create
\return created compressor object or nullptr if the compressor is unable to be created
*/
Compressor* CompressorStream::CreateCompressor(AZ::u32 compressorId)
{
    m_compressor.reset(nullptr);
    if (compressorId == CompressorZLib::TypeId())
    {
        m_compressor.reset(aznew CompressorZLib);
    }
    else if (compressorId == CompressorZStd::TypeId())
    {
        m_compressor.reset(aznew CompressorZStd);
    }
    else
    {
        AZ_Assert(false, "Unable to create compressor with type id [0x%08x]", compressorId);
    }

    return m_compressor.get();
}

} // namespace AZ::IO
