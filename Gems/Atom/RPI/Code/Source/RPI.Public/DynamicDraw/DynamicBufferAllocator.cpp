/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/DynamicDraw/DynamicBuffer.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>

namespace AZ
{
    namespace RPI
    {
        void DynamicBufferAllocator::Init(uint32_t ringBufferSize)
        {
            if (m_bufferData.IsCurrentBufferValid())
            {
                AZ_Assert(false, "DynamicBufferAllocator was already initialized");
                return;
            }

            m_ringBufferSize = ringBufferSize;

            for (unsigned i{ 0 }; i < m_bufferData.GetElementCount(); i++)
            {
                m_bufferData.CreateOrResizeCurrentBuffer(m_ringBufferSize);
                m_bufferStartAddresses.GetCurrentElement() = m_bufferData.GetCurrentElement()->Map(m_ringBufferSize, 0);
                m_bufferData.AdvanceCurrentElement();
                m_bufferStartAddresses.AdvanceCurrentElement();
            }
        }

        void DynamicBufferAllocator::Shutdown()
        {
            for (unsigned i{ 0 }; i < m_bufferData.GetElementCount(); i++)
            {
                m_bufferData.AdvanceCurrentElement()->Unmap();
                m_bufferStartAddresses.AdvanceCurrentElement().clear();
            }
        }

        // [GFX TODO][ATOM-13182] Add unit tests for DynamicBufferAllocator's Allocate function
        RHI::Ptr<DynamicBuffer> DynamicBufferAllocator::Allocate(uint32_t size, uint32_t alignment)
        {
            // m_bufferData can be invalid for Null back end
            if (m_bufferStartAddresses.GetCurrentElement().empty() || !m_bufferStartAddresses.GetCurrentElement().begin()->second)
            {
                return nullptr;
            }

            m_currentPosition = RHI::AlignUp(m_currentPosition, alignment);

            if (size > m_ringBufferSize)
            {
                AZ_WarningOnce(
                    "RPI",
                    !m_enableAllocationWarning,
                    "DynamicBufferAllocator::Allocate: try to allocate buffer which size is larger than the ring buffer size");
                return nullptr;
            }

            // Return if the allocation of current frame has reached limit
            if (m_currentPosition + size > m_ringBufferSize)
            {
                AZ_WarningOnce("RPI", !m_enableAllocationWarning, "DynamicBufferAllocator::Allocate: no more buffer space is available");
                return nullptr;
            }

            RHI::Ptr<DynamicBuffer> allocatedBuffer = aznew DynamicBuffer();
            for(auto [deviceIndex, address] : m_bufferStartAddresses.GetCurrentElement())
            {
                allocatedBuffer->m_address[deviceIndex] = (uint8_t*)address + m_currentPosition;
            }
            allocatedBuffer->m_size = size;
            allocatedBuffer->m_allocator = this;

            m_currentPosition += size;

            return allocatedBuffer;
        }

        RHI::IndexBufferView DynamicBufferAllocator::GetIndexBufferView(RHI::Ptr<DynamicBuffer> dynamicBuffer, RHI::IndexFormat format)
        {
            return RHI::IndexBufferView(
                *m_bufferData.GetCurrentElement()->GetRHIBuffer(), GetBufferAddressOffset(dynamicBuffer), dynamicBuffer->m_size, format);
        }

        RHI::StreamBufferView DynamicBufferAllocator::GetStreamBufferView(RHI::Ptr<DynamicBuffer> dynamicBuffer, uint32_t strideByteCount)
        {
            return RHI::StreamBufferView(
                *m_bufferData.GetCurrentElement()->GetRHIBuffer(),
                GetBufferAddressOffset(dynamicBuffer),
                dynamicBuffer->m_size,
                strideByteCount);
        }

        uint32_t DynamicBufferAllocator::GetBufferAddressOffset(RHI::Ptr<DynamicBuffer> dynamicBuffer)
        {
            auto it = m_bufferStartAddresses.GetCurrentElement().begin();

            return aznumeric_cast<uint32_t>((uint8_t*)dynamicBuffer->m_address[it->first] - (uint8_t*)it->second);
        }

        void DynamicBufferAllocator::SetEnableAllocationWarning(bool enable)
        {
            m_enableAllocationWarning = enable;
        }

        void DynamicBufferAllocator::FrameEnd()
        {
            m_bufferData.AdvanceCurrentElement();
            m_bufferStartAddresses.AdvanceCurrentElement();
            m_currentPosition = 0;
        }
    } // namespace RPI
} // namespace AZ
