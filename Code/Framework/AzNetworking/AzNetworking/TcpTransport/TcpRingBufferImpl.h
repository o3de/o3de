/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <stdint.h>
#include <string.h> // For memmove

namespace AzNetworking
{
    //! @class TcpRingBufferImpl
    //! @brief helper class to move ring buffer implementation details out of template header code.
    class TcpRingBufferImpl
    {
    public:

        //! Construct with a buffer and size.
        //! @param buffer     input buffer to use as ring-buffer storage
        //! @param bufferSize size of the input buffer in bytes
        TcpRingBufferImpl(uint8_t* buffer, uint32_t bufferSize);

        virtual ~TcpRingBufferImpl() = default;

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

        //! Returns the number of contiguous bytes free for writing.
        //! @return number of contiguous bytes free for writing
        uint32_t GetFreeBytes() const;

        //! Returns the number of bytes of data valid for reading.
        //! @return number of bytes of data valid for reading
        uint32_t GetUsedBytes() const;

        uint8_t* m_bufferStart;
        uint8_t* m_bufferEnd;
        uint8_t* m_writePtr;
        uint8_t* m_readPtr;
    };
}

#include <AzNetworking/TcpTransport/TcpRingBufferImpl.inl>
