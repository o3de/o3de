/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

