/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    template <uint32_t SIZE>
    inline TcpRingBuffer<SIZE>::TcpRingBuffer()
        : m_impl(m_buffer.data(), m_buffer.size())
    {
        ;
    }

    template <uint32_t SIZE>
    inline uint8_t* TcpRingBuffer<SIZE>::ReserveBlockForWrite(uint32_t numBytes)
    {
        return m_impl.ReserveBlockForWrite(numBytes);
    }

    template <uint32_t SIZE>
    inline uint8_t* TcpRingBuffer<SIZE>::GetReadBufferData() const
    {
        return m_impl.GetReadBufferData();
    }

    template <uint32_t SIZE>
    inline uint32_t TcpRingBuffer<SIZE>::GetReadBufferSize() const
    {
        return m_impl.GetReadBufferSize();
    }

    template <uint32_t SIZE>
    inline bool TcpRingBuffer<SIZE>::AdvanceWriteBuffer(uint32_t numBytes)
    {
        return m_impl.AdvanceWriteBuffer(numBytes);
    }

    template <uint32_t SIZE>
    inline bool TcpRingBuffer<SIZE>::AdvanceReadBuffer(uint32_t numBytes)
    {
        return m_impl.AdvanceReadBuffer(numBytes);
    }
}
