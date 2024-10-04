/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBuffer.h>

namespace AZ
{
    namespace RPI
    {
        bool DynamicBuffer::Write(const void* data, uint32_t size)
        {
            if (m_size >= size)
            {
                for(auto& [deviceIndex, address] : m_address)
                {
                    memcpy(address, data, size);
                }
                return true;
            }
            AZ_Assert(false, "Can't write data out of range");
            return false;
        }

        uint32_t DynamicBuffer::GetSize()
        {
            return m_size;
        }

        const AZStd::unordered_map<int, void*>& DynamicBuffer::GetBufferAddress()
        {
            return m_address;
        }

        RHI::IndexBufferView DynamicBuffer::GetIndexBufferView(RHI::IndexFormat format)
        {
            return m_allocator->GetIndexBufferView(this, format);
        }

        RHI::StreamBufferView DynamicBuffer::GetStreamBufferView(uint32_t strideByteCount)
        {
            return m_allocator->GetStreamBufferView(this, strideByteCount);
        }

        void DynamicBuffer::Initialize(const AZStd::unordered_map<int, void*>& address, uint32_t size)
        {
            m_address = address;
            m_size = size;
        }
    }
}
