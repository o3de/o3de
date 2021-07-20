/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpRingBufferImpl.h>

namespace AzNetworking
{
    TcpRingBufferImpl::TcpRingBufferImpl(uint8_t* buffer, uint32_t bufferSize)
        : m_bufferStart(buffer)
        , m_bufferEnd(buffer + bufferSize)
        , m_writePtr(buffer)
        , m_readPtr(buffer)
    {
        ;
    }

    uint8_t* TcpRingBufferImpl::ReserveBlockForWrite(uint32_t numBytes)
    {
        // If we don't have enough space remaining, pack the ring buffer
        if (GetFreeBytes() < numBytes)
        {
            const uint32_t numUsedBytes = GetUsedBytes();
            memmove(m_bufferStart, m_readPtr, numUsedBytes);
            m_writePtr = m_bufferStart + numUsedBytes;
            m_readPtr = m_bufferStart;
        }

        if (GetFreeBytes() < numBytes)
        {
            return nullptr;
        }

        return m_writePtr;
    }

    bool TcpRingBufferImpl::AdvanceWriteBuffer(uint32_t numBytes)
    {
        if (GetFreeBytes() < numBytes)
        {
            return false;
        }

        m_writePtr += numBytes;
        return true;
    }

    bool TcpRingBufferImpl::AdvanceReadBuffer(uint32_t numBytes)
    {
        if (numBytes > GetUsedBytes())
        {
            return false;
        }

        m_readPtr += numBytes;
        return true;
    }
}
