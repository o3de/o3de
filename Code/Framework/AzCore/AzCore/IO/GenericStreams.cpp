/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ::IO
{

        /*!
        \brief Default implementation of WriteFromStream, this will transfer bytes from any generic input stream to this stream.
        Any stream that writes into a preexisting memory buffer can implement a more efficient version of this method that
        directly passes the memory buffer into the inputStream to save one memory copy of the data.
        \param bytes Amount of bytes to write
        \param inputStream The stream to read data from
        */
        SizeType GenericStream::WriteFromStream(SizeType bytes, GenericStream* inputStream)
        {
            char tempBuffer[StreamToStreamCopyBufferSize];
            SizeType totalBytesWritten = 0;

            AZ_Assert(inputStream, "Cannot copy from a null input stream.");
            AZ_Assert(inputStream != this, "Can't write and read from the same stream.");

            // Loop through the total number of bytes requested, reading a chunk of it at a time from the inputStream
            // into tempBuffer, then writing it out into this stream.
            for (SizeType windowOffset = 0; windowOffset < bytes; windowOffset += StreamToStreamCopyBufferSize)
            {
                // Read in the appropriate number of bytes on every loop iteration.
                // (ex:  515 bytes requested will read 256 bytes, 256 bytes, and 3 bytes)
                SizeType transferBytes = AZStd::GetMin(bytes - windowOffset, aznumeric_cast<SizeType>(StreamToStreamCopyBufferSize));

                SizeType bytesRead = inputStream->Read(transferBytes, tempBuffer);
                if (bytesRead > 0)
                {
                    totalBytesWritten += Write(bytesRead, tempBuffer);
                }
            }

            return totalBytesWritten;
        }

    /*!
    \brief Reads from the stream using the specified offset
    \param bytes Amount of bytes to read
    \param oBuffer Buffer to read bytes into. This buffer must be at least sizeof(@bytes)
    \param offset Offset to seek to before reading. If negative no seeking occurs
    */
    SizeType GenericStream::ReadAtOffset(SizeType bytes, void* oBuffer, OffsetType offset)
    {
        if (offset >= 0)
        {
            Seek(offset, SeekMode::ST_SEEK_BEGIN);
        }
        return Read(bytes, oBuffer);
    }

    /*!
    \brief Writes to the stream using the specified offset
    \param bytes Amount of bytes to write
    \param iBuffer Buffer containing data to write
    \param offset Offset to seek to before writing. If negative no seeking occurs
    */
    SizeType GenericStream::WriteAtOffset(SizeType bytes, const void* iBuffer, OffsetType offset)
    {
        if (offset >= 0)
        {
            Seek(offset, SeekMode::ST_SEEK_BEGIN);
        }
        return Write(bytes, iBuffer);
    }

    SizeType GenericStream::ComputeSeekPosition(OffsetType bytes, SeekMode mode)
    {
        SizeType absBytes = static_cast<SizeType>(abs(bytes));
        if (mode == ST_SEEK_BEGIN)
        {
            if (bytes > 0)
            {
                return absBytes;
            }
            else
            {
                return 0;
            }
        }
        else if (mode == ST_SEEK_CUR)
        {
            SizeType curPos = GetCurPos();
            if (bytes > 0)
            {
                return curPos + absBytes;
            }
            else
            {
                if (curPos >= absBytes)
                {
                    return curPos - absBytes;
                }
                else
                {
                    return 0;
                }
            }
        }
        else // mode == ST_SEEK_END
        {
            SizeType curLen = GetLength();
            if (bytes > 0)
            {
                return curLen + absBytes;
            }
            else
            {
                if (curLen >= absBytes)
                {
                    return curLen - absBytes;
                }
                else
                {
                    return 0;
                }
            }
        }
    }

        /*
         * StreamerStream
         */


    /*
     * SystemFileStream
     */
    SystemFileStream::SystemFileStream()
        : m_mode(OpenMode::Invalid)
    {}

    SystemFileStream::SystemFileStream(SystemFile&& file)
        : SystemFileStream(AZStd::move(file), OpenMode::Invalid)
    {
    }

    SystemFileStream::SystemFileStream(SystemFile&& file, OpenMode mode)
        : m_file(AZStd::move(file))
        , m_mode(mode)
    {
    }

    SystemFileStream::SystemFileStream(const char* path, OpenMode mode)
    {
        Open(path, mode);
    }

    // Let the SystemFile destructor close or not close any files based
    // on its destruction logic
    SystemFileStream::~SystemFileStream() = default;

    // Default the move constructor and assignment operator
    SystemFileStream::SystemFileStream(SystemFileStream&&) = default;
    SystemFileStream& SystemFileStream::operator=(SystemFileStream&&) = default;

    bool SystemFileStream::IsOpen() const
    {
        return m_file.IsOpen();
    }

    bool SystemFileStream::CanSeek() const
    {
        return true;
    }

    bool SystemFileStream::CanRead() const
    {
        return true;
    }

    bool SystemFileStream::CanWrite() const
    {
        return true;
    }

    bool SystemFileStream::Open(const char* path, OpenMode mode)
    {
        m_mode = mode;
        Close();
        return m_file.Open(path, TranslateOpenModeToSystemFileMode(path, mode));
    }

    void SystemFileStream::Close()
    {
        m_file.Close();
    }

    static SystemFile::SeekMode ConvertStreamToSystemFileSeekMode(GenericStream::SeekMode mode)
    {
        switch (mode)
        {
            case GenericStream::SeekMode::ST_SEEK_BEGIN:
                return SystemFile::SeekMode::SF_SEEK_BEGIN;
            case GenericStream::SeekMode::ST_SEEK_CUR:
                return SystemFile::SeekMode::SF_SEEK_CURRENT;
            case GenericStream::SeekMode::ST_SEEK_END:
                return SystemFile::SeekMode::SF_SEEK_END;
            default:
                AZ_Assert(false, "Invalid Seek mode specified %d", static_cast<int>(mode));
                break;
        }

        return SystemFile::SeekMode::SF_SEEK_BEGIN;
    }

    void SystemFileStream::Seek(OffsetType bytes, SeekMode mode)
    {
        m_file.Seek(bytes, ConvertStreamToSystemFileSeekMode(mode));
    }

    SizeType SystemFileStream::GetCurPos() const
    {
        return m_file.Tell();
    }

    SizeType SystemFileStream::Read(SizeType bytes, void* oBuffer)
    {
        return m_file.Read(bytes, oBuffer);
    }

    SizeType SystemFileStream::Write(SizeType bytes, const void* iBuffer)
    {
        return m_file.Write(iBuffer, bytes);
    }

    SizeType SystemFileStream::GetLength() const
    {
        return m_file.Length();
    }

    const char* SystemFileStream::GetFilename() const
    {
        return m_file.Name();
    }

    OpenMode SystemFileStream::GetModeFlags() const
    {
        return m_mode;
    }

    bool SystemFileStream::ReOpen()
    {
        AZ_Assert(m_mode != OpenMode::Invalid, "File must have been opened at least once with valid OpenMode flags in order to be reopened");
        return Open(m_file.Name(), m_mode);
    }

    SystemFile SystemFileStream::MoveSystemFile() &&
    {
        return AZStd::move(m_file);
    }


    /*
     * MemoryStream
     */
    void MemoryStream::Seek(OffsetType bytes, SeekMode mode)
    {
        m_curOffset = static_cast<size_t>(ComputeSeekPosition(bytes, mode));
    }

    SizeType MemoryStream::Read(SizeType bytes, void* oBuffer)
    {
        if (m_curOffset >= GetLength())
        {
            return 0;
        }
        SizeType bytesLeft = GetLength() - m_curOffset;
        if (bytes > bytesLeft)
        {
            bytes = bytesLeft;
        }
        if (bytes)
        {
            memcpy(oBuffer, m_buffer + m_curOffset, static_cast<size_t>(bytes));
            m_curOffset += static_cast<size_t>(bytes);
        }
        return bytes;
    }

    SizeType MemoryStream::PrepareForWrite(SizeType bytes)
    {
        AZ_Assert(m_mode == MSM_READWRITE, "This memory stream is not writable!");
        if (m_curOffset >= m_bufferLen)
        {
            return 0;
        }
        SizeType bytesLeft = m_bufferLen - m_curOffset;
        if (bytes > bytesLeft)
        {
            bytes = bytesLeft;
        }
            return bytes;
        }

    SizeType MemoryStream::Write(SizeType bytes, const void* iBuffer)
    {
        bytes = PrepareForWrite(bytes);
        if (bytes)
        {
            memcpy(const_cast<char*>(m_buffer) + m_curOffset, iBuffer, static_cast<size_t>(bytes));
            m_curOffset += static_cast<size_t>(bytes);
            m_curLen = AZStd::GetMax(m_curOffset, m_curLen);
        }
        return bytes;
    }

    SizeType MemoryStream::WriteFromStream(SizeType bytes, GenericStream *inputStream)
    {
        AZ_Assert(inputStream, "Cannot copy from a null input stream.");
        AZ_Assert(inputStream != this, "Can't write and read from the same stream.");

        bytes = PrepareForWrite(bytes);
        if (bytes)
        {
            bytes = inputStream->Read(bytes, const_cast<char*>(m_buffer) + m_curOffset);
            m_curOffset += static_cast<size_t>(bytes);
            m_curLen = AZStd::GetMax(m_curOffset, m_curLen);
        }
        return bytes;
    }

    /*
     * StdoutStream - Implementation of GenericStream interface
     * for stdout file descriptor
     */

    StdoutStream::~StdoutStream()
    {
        std::fflush(stdout);
    }
    bool StdoutStream::IsOpen() const
    {
        return true;
    }

    bool StdoutStream::CanSeek() const
    {
        return false;
    }

    bool StdoutStream::CanRead() const
    {
        return false;
    }

    bool StdoutStream::CanWrite() const
    {
        return true;
    }

    void StdoutStream::Seek(AZ::IO::OffsetType, SeekMode)
    {
        AZ_Assert(false, "Cannot seek in stdout stream");
    }

    AZ::IO::SizeType StdoutStream::Read(AZ::IO::SizeType, void*)
    {
        AZ_Assert(false, "The stdout file handle cannot be read from");
        return 0;
    }

    AZ::IO::SizeType StdoutStream::Write(AZ::IO::SizeType bytes, const void* iBuffer)
    {
        return fwrite(iBuffer, 1, bytes, stdout);
    }

    AZ::IO::SizeType StdoutStream::GetCurPos() const
    {
        return 0;
    }

    AZ::IO::SizeType StdoutStream::GetLength() const
    {
        return 0;
    }

    AZ::IO::OpenMode StdoutStream::GetModeFlags() const
    {
        // Avoid including FileIO.h to retrieve values for OpenMode enum
        constexpr AZ::IO::OpenMode modeWrite{ 1 << 1 };
        return modeWrite;
    }

    const char* StdoutStream::GetFilename() const
    {
        return "<stdout>";
    }
}   // namespace AZ::IO
