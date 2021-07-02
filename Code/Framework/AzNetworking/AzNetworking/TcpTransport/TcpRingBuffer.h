/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/TcpTransport/TcpRingBufferImpl.h>

namespace AzNetworking
{
    //! @class TcpRingBuffer
    //! @brief statically sized ringbuffer class for reading from or writing to data streams like a TCP socket connection.
    template <uint32_t SIZE>
    class TcpRingBuffer
    {
    public:

        TcpRingBuffer();
        ~TcpRingBuffer() = default;

        //! Returns a pointer into writable memory guaranteed to be of at least numBytes in length.
        //! @param numBytes maximum number of bytes to be written to the ring-buffer
        //! @return pointer to the requested memory, nullptr if the requested size is too large for the ringbuffer to store contiguously
        uint8_t* ReserveBlockForWrite(uint32_t numBytes);

        //! Returns the start of ringbuffer read memory.
        //! @return pointer to the start of ringbuffer read memory
        uint8_t* GetReadBufferData() const;

        //! Returns the size of ringbuffer read memory in bytes.
        //! @return the size of ringbuffer read memory in bytes
        uint32_t GetReadBufferSize() const;

        //! Advances the ringbuffer write offset by the requested number of bytes.
        //! @param numBytes number of bytes to advance the ringbuffer write pointer by
        //! @return boolean true on success
        bool AdvanceWriteBuffer(uint32_t numBytes);

        //! Advances the ringbuffer read offset by the requested number of bytes.
        //! @param numBytes number of bytes to advance the ringbuffer read pointer by
        //! @return boolean true on success
        bool AdvanceReadBuffer(uint32_t numBytes);

    private:

        AZStd::array<uint8_t, SIZE> m_buffer;
        TcpRingBufferImpl m_impl;
    };
}

#include <AzNetworking/TcpTransport/TcpRingBuffer.inl>
