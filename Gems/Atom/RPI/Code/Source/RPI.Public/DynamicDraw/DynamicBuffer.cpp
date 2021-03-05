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

#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBuffer.h>

namespace AZ
{
    namespace RPI
    {
        bool DynamicBuffer::Write(void* data, uint32_t size)
        {
            if (m_size >= size)
            {
                memcpy(m_address, data, size);
                return true;
            }
            AZ_Assert(false, "Can't write data out of range");
            return false;
        }

        uint32_t DynamicBuffer::GetSize()
        {
            return m_size;
        }

        void* DynamicBuffer::GetBufferAddress()
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

        void DynamicBuffer::Initialize(void* address, uint32_t size)
        {
            m_address = address;
            m_size = size;
        }
    }
}
