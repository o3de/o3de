/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline uint8_t* TcpRingBufferImpl::GetReadBufferData() const
    {
        return m_readPtr;
    }

    inline uint32_t TcpRingBufferImpl::GetReadBufferSize() const
    {
        return GetUsedBytes();
    }

    inline uint32_t TcpRingBufferImpl::GetFreeBytes() const
    {
        return static_cast<uint32_t>(m_bufferEnd - m_writePtr);
    }

    inline uint32_t TcpRingBufferImpl::GetUsedBytes() const
    {
        return static_cast<uint32_t>(m_writePtr - m_readPtr);
    }
}

