/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::IO
{
    struct BufferedSystemFileStreamConfig
    {
        //! Sets the default buffer size for the BufferedSystemFileStream
        //! Used when the buffer size parameter is not supplied
        //! Values in the range of [0, 2^31 - 1) are valid
        static inline AZ::s32 constexpr DefaultWriteBufferSize = 4096;
        //! Default Read BufferSize to 0
        //! Since reading from a SystemFile normally is not a bottleneck
        static inline AZ::s32 constexpr DefaultReadBufferSize = 0;

        //! If the write buffer size <=0, writes are unbuffered
        AZ::s32 m_writeBufferSize = DefaultWriteBufferSize;
        //! If the read buffer size <=0, reads are unbuffered
        AZ::s32 m_readBufferSize = DefaultReadBufferSize;
    };
    //! Implementation of a SystemFileStream with support for buffering reads and writes
    //! Supports a max buffer size of up to 2GiB
    //! The buffer is pre-allocated during construction or when SetBufferSize is invoked
    //! The implementation of the buffered operations are not thread safe and require
    //! synchronization primitives around them if thread safety is desired
    class BufferedSystemFileStream
        : public SystemFileStream
    {
    public:
        //! Creates a Buffered SystemFile stream using the Default buffer size
        BufferedSystemFileStream();
        BufferedSystemFileStream(BufferedSystemFileStreamConfig bufferConfig);
        //! Same as the SystemFileStream overloads, accept an additional bufferSize parameter is supported
        BufferedSystemFileStream(const char* path, OpenMode mode, BufferedSystemFileStreamConfig bufferConfig);
        //! SystemFiles are only movable and only one instance is available at a time.
        BufferedSystemFileStream(SystemFile&& file, BufferedSystemFileStreamConfig bufferConfig);
        //! For this overload, the mode parameter
        //! is for passing the mode flags used to open the supplied SystemFile
        //! This purpose of the mmode parameter is only for use in the ReOpen() function
        //! to determine how to reopen the SystemFile
        BufferedSystemFileStream(SystemFile&& file, OpenMode mode, BufferedSystemFileStreamConfig bufferConfig);
        ~BufferedSystemFileStream() override;

        // Bring in the base class constructors as well
        using SystemFileStream::SystemFileStream;

        //! Flush the buffer and calls the SystemFilestream Close method
        void Close() override;

        void Seek(OffsetType bytes, SeekMode mode) override;

        //! Reads the data into the specified output buffer.
        //! If the read size in bytes is smaller than the read size buffer
        //! than additional bytes will attempt to be read into the internal
        //! read buffer to reduce the number of underlying read operations
        SizeType Read(SizeType bytes, void* oBuffer) override;

        //! Writes the data to the internal write buffer if the input data
        //! can fit within it.
        //! Once the write buffer is full, the contents are flushed to disk
        SizeType Write(SizeType bytes, const void* iBuffer) override;

        //! Flush the current m_writeBuffer data to the System File
        //! and clears the m_writeBuffer data
        void Flush() override;

        //! Updates the buffer configuration associated with the stream
        //! If the buffer configuration results in a smaller write buffer size,
        //! then a Flush operation is performed
        void SetBufferConfig(BufferedSystemFileStreamConfig bufferConfig);

    private:
        BufferedSystemFileStreamConfig m_bufferConfig;
        AZStd::vector<AZStd::byte> m_writeBuffer;
        AZStd::vector<AZStd::byte> m_readBuffer;
    };

}   // namespace AZ::IO
