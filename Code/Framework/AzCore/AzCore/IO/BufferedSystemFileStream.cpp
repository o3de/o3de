/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/BufferedSystemFileStream.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

namespace AZ::IO
{
    BufferedSystemFileStream::BufferedSystemFileStream()
        : BufferedSystemFileStream{ BufferedSystemFileStreamConfig{}}
    {
    }

    BufferedSystemFileStream::BufferedSystemFileStream(BufferedSystemFileStreamConfig config)
    {
        // Delegate to the SetBufferConfig function to update the reserve size of the buffers
        SetBufferConfig(AZStd::move(config));
    }

    BufferedSystemFileStream::BufferedSystemFileStream(SystemFile&& file, BufferedSystemFileStreamConfig config)
        : SystemFileStream(AZStd::move(file))
    {
        SetBufferConfig(AZStd::move(config));
    }

    BufferedSystemFileStream::BufferedSystemFileStream(SystemFile&& file, OpenMode mode, BufferedSystemFileStreamConfig config)
        : SystemFileStream(AZStd::move(file), mode)
    {
        SetBufferConfig(AZStd::move(config));
    }

    BufferedSystemFileStream::BufferedSystemFileStream(const char* path, OpenMode mode, BufferedSystemFileStreamConfig config)
        : SystemFileStream(path, mode)
    {
        SetBufferConfig(AZStd::move(config));
    }

    BufferedSystemFileStream::~BufferedSystemFileStream()
    {
        Close();
    }

    void BufferedSystemFileStream::Close()
    {
        Flush();
        SystemFileStream::Close();
    }


    void BufferedSystemFileStream::Seek(OffsetType bytes, SeekMode mode)
    {
        // Flush occurs on Seek as it may change the current file position
        Flush();
        SystemFileStream::Seek(bytes, mode);
    }


    SizeType BufferedSystemFileStream::Read(SizeType bytes, void* oBuffer)
    {
        // Find the max amount of bytes that should be read into the buffer
        // Read at minimum the read buffer config size
        const SizeType systemFileBytesToRead = AZStd::max(bytes, static_cast<SizeType>(m_bufferConfig.m_readBufferSize));
        auto byteOutputBuffer = reinterpret_cast<AZStd::byte*>(oBuffer);

        if (m_readBuffer.empty())
        {
            // If the number of bytes to read cannot fit within the read buffer,
            // read directly into the output buffer
            if (systemFileBytesToRead > m_readBuffer.capacity())
            {
                return SystemFileStream::Read(systemFileBytesToRead, oBuffer);
            }
            else
            {
                // Read into the readBuffer first, and then into the output buffer
                m_readBuffer.resize_no_construct(systemFileBytesToRead);
                const SizeType bytesReadFromSystemFile = SystemFileStream::Read(systemFileBytesToRead, m_readBuffer.data());
                // Resize the read buffer to the exact amount of bytes read
                m_readBuffer.resize_no_construct(bytesReadFromSystemFile);
                const SizeType bytesToStore = AZStd::min(bytesReadFromSystemFile, bytes);
                // Copy up the number of bytes read into the output buffer
                AZStd::ranges::copy(AZStd::span(m_readBuffer.data(), bytesToStore), byteOutputBuffer);
                m_readBuffer.erase(m_readBuffer.begin(), m_readBuffer.begin() + bytesToStore);

                return bytesToStore;
            }
        }

        // The read buffer has data in it
        // so read from it up until the bytes value.
        const SizeType bytesReadFromBuffer = AZStd::min(bytes, m_readBuffer.size());

        // ranges copy conveniently returns a pair of iterators that is past the elements copied from the input iterator to the output iterator
        auto [postReadBufferIter, systemFileReadOutputIter] = AZStd::ranges::copy(AZStd::span(m_readBuffer.data(), bytesReadFromBuffer), byteOutputBuffer);
        // Decreases bytes by the amount of bytes read from read buffer
        bytes -= bytesReadFromBuffer;

        if (systemFileBytesToRead > m_readBuffer.capacity())
        {
            // Read the rest of the bytes from the underlying SystemFile
            // into the area after the buffered read
            return bytesReadFromBuffer + SystemFileStream::Read(systemFileBytesToRead, systemFileReadOutputIter);
        }
        else
        {
            m_readBuffer.resize_no_construct(systemFileBytesToRead);
            const SizeType bytesReadFromSystemFile = SystemFileStream::Read(systemFileBytesToRead, m_readBuffer.data());
            // Resize the read buffer to the exact amount of bytes read
            m_readBuffer.resize_no_construct(bytesReadFromSystemFile);

            const SizeType bytesToStore = AZStd::min(bytesReadFromSystemFile, bytes);
            // Copy up the number of bytes read into the storage area after buffered read
            AZStd::ranges::copy(AZStd::span(m_readBuffer.data(), bytesToStore), systemFileReadOutputIter);
            m_readBuffer.erase(m_readBuffer.begin(), m_readBuffer.begin() + bytesToStore);

            return bytesReadFromBuffer + bytesReadFromSystemFile;
        }
    }

    SizeType BufferedSystemFileStream::Write(SizeType bytes, const void* iBuffer)
    {
        // The strategy for this function is that it enforces no dynamic memory allocations
        // in the write buffer, while prioritizing reducing the number of Write
        // calls buffer is filled with data to push it over the write buffer size value

        // If the write buffer is empty, check if the data can be stored within the buffer
        if (m_writeBuffer.empty())
        {
            // If the number of bytes to write is greater than write buffer max
            // size then write directly to the file
            if (bytes > m_bufferConfig.m_writeBufferSize)
            {
                return SystemFileStream::Write(bytes, iBuffer);
            }
            else
            {
                m_writeBuffer.insert(m_writeBuffer.end(), reinterpret_cast<const AZStd::byte*>(iBuffer),
                    reinterpret_cast<const AZStd::byte*>(iBuffer) + bytes);
                // All the bytes have been buffered so return the value
                return bytes;
            }
        }

        // The write buffer is not empty in this case so the logic is a bit more complex
        // If the new amount of added bytes can fit within the buffer append the input data to the buffer
        // This reduces the number of potential write operations to 1
        if (bytes + m_writeBuffer.size() < m_writeBuffer.capacity())
        {
            m_writeBuffer.insert(m_writeBuffer.end(), reinterpret_cast<const AZStd::byte*>(iBuffer),
                reinterpret_cast<const AZStd::byte*>(iBuffer) + bytes);

            // If the write buffer is now larger than the configured size, flush it to the underlying
            // SystemFile
            if (m_writeBuffer.size() > m_bufferConfig.m_writeBufferSize)
            {
                Flush();
            }
            return bytes;
        }
        else
        {
            // In this case the input data + the current data write buffer cannot fit
            // without needing to reallocate the write buffer, so flush the write buffer and directly write out the input data
            Flush();
            // Return only input data bytes written, as previous calls to Write() will have returned
            // the bytes written to the write buffer
            return SystemFileStream::Write(bytes, iBuffer);
        }
    }

    void BufferedSystemFileStream::Flush()
    {
        if (!m_writeBuffer.empty())
        {
            const SizeType bytesWritten = SystemFileStream::Write(m_writeBuffer.size(), m_writeBuffer.data());
            // Reduce the m_writeBuffer by the number of bytes written
            m_writeBuffer.erase(m_writeBuffer.begin(), m_writeBuffer.begin() + bytesWritten);
        }
    }

    void BufferedSystemFileStream::SetBufferConfig(BufferedSystemFileStreamConfig bufferConfig)
    {
        m_bufferConfig = AZStd::move(bufferConfig);

        // The old buffer configuration has been swapped to the input parameter
        // If the new write buffer max size is less than the current size, than flush the write buffer to disk
        if (m_bufferConfig.m_writeBufferSize < m_writeBuffer.size())
        {
            Flush();
        }

        if (m_bufferConfig.m_writeBufferSize == 0)
        {
            // If the write buffer size config is zero, writes are unbuffered
            // completely clear out the memory allocated to the write buffer
            m_writeBuffer = {};
        }
        else
        {
            // reserve space in the write buffer
            constexpr size_t WriteBufferSlackSize = 4096;
            // 4K slack is added to help with reducing the number read and write calls to the underlying SystemFile
            // when a Write() call causes the data in the buffer to go a bit beyond the configured buffer size
            m_writeBuffer.reserve(m_bufferConfig.m_writeBufferSize + WriteBufferSlackSize);
        }

        if (m_bufferConfig.m_readBufferSize == 0)
        {
            // reads are unbuffered in this when the read buffer config size is <= 0
            m_readBuffer = {};
        }
        else
        {
            m_readBuffer.reserve(m_bufferConfig.m_readBufferSize);
        }
    }

}   // namespace AZ::IO
