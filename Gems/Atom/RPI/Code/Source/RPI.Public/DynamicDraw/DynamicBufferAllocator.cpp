/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBuffer.h>

namespace AZ
{
    namespace RPI
    {
        void DynamicBufferAllocator::Init(uint32_t ringBufferSize)
        {
            if (m_ringBuffer)
            {
                AZ_Assert(false, "DynamicBufferAllocator was already initialized");
                return;
            }

            // Create the ring buffer from common pool
            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::DynamicInputAssembly;
            desc.m_bufferName = "DyanmicBufferRing";
            desc.m_elementSize = 1;
            desc.m_byteCount = ringBufferSize;
            m_ringBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

            if (m_ringBuffer.get() == nullptr)
            {
                AZ_Assert(false, "Failed to initialize DyanmicBufferAllocator");
                return;
            }
            
            m_ringBufferSize = ringBufferSize;
            m_ringBufferStartAddress = m_ringBuffer->Map(m_ringBufferSize, 0);
            
            m_currentPosition = 0;
            m_endPositionLimit = 0;
            m_currentFrame = 0;
            for (uint32_t frame = 0; frame < AZ::RHI::Limits::Device::FrameCountMax; frame++)
            {
                m_frameStartPositions[frame] = 0;
            }
        }

        void DynamicBufferAllocator::Shutdown()
        {
            m_ringBuffer->Unmap();
            m_ringBuffer = nullptr;
            m_ringBufferStartAddress = nullptr;
        }

        // [GFX TODO][ATOM-13182] Add unit tests for DynamicBufferAllocator's Allocate function 
        RHI::Ptr<DynamicBuffer> DynamicBufferAllocator::Allocate(uint32_t size, [[maybe_unused]]uint32_t alignment)
        {
            size = RHI::AlignUp(size, alignment);
            uint32_t allocatePosition = 0;

            //m_ringBufferStartAddress can be null for Null back end
            if (!m_ringBufferStartAddress)
            {
                return nullptr;
            }

            if (size > m_ringBufferSize)
            {
                AZ_WarningOnce("RPI", !m_enableAllocationWarning, "DynamicBufferAllocator::Allocate: try to allocate buffer which size is larger than the ring buffer size");
                return nullptr;
            }

            // Return if the allocation of current frame has reached limit
            if (m_endPositionLimit == m_currentPosition && m_currentAllocatedSize > 0)
            {
                AZ_WarningOnce("RPI", !m_enableAllocationWarning, "DynamicBufferAllocator::Allocate: no more buffer is available");
                return nullptr;
            }

            if (m_endPositionLimit > m_currentPosition)
            {
                if (m_endPositionLimit - m_currentPosition >= size)
                {
                    allocatePosition = m_currentPosition;
                    m_currentPosition += size;
                }
                else
                {
                    AZ_WarningOnce("RPI", !m_enableAllocationWarning, "DynamicBufferAllocator::Allocate: requested size (%d bytes) is larger than the size left (%d bytes)", size, m_endPositionLimit - m_currentPosition);
                    return nullptr;
                }
            }
            else
            {
                if (m_ringBufferSize - m_currentPosition >= size)
                {
                    allocatePosition = m_currentPosition;
                    m_currentPosition += size;
                    if (m_ringBufferSize == m_currentPosition)
                    {
                        m_currentPosition = 0;
                    }
                }
                else
                {
                    if (m_endPositionLimit >= size)
                    {
                        allocatePosition = 0;
                        m_currentPosition = size;
                    }
                    else
                    {
                        AZ_WarningOnce("RPI", !m_enableAllocationWarning, "DynamicBufferAllocator::Allocate: requested size (%d bytes) is larger than the size left (%d bytes)", size, m_endPositionLimit);
                        return nullptr;
                    }
                }
            }

            m_currentAllocatedSize += size;

            RHI::Ptr<DynamicBuffer> allocatedBuffer = aznew DynamicBuffer();
            allocatedBuffer->m_address = (uint8_t*)m_ringBufferStartAddress + allocatePosition;
            allocatedBuffer->m_size = size;
            allocatedBuffer->m_allocator = this;
            return allocatedBuffer;
        }

        RHI::SingleDeviceIndexBufferView DynamicBufferAllocator::GetIndexBufferView(RHI::Ptr<DynamicBuffer> dynamicBuffer, RHI::IndexFormat format)
        {
            return RHI::SingleDeviceIndexBufferView(
                *m_ringBuffer->GetRHIBuffer(),
                GetBufferAddressOffset(dynamicBuffer),
                dynamicBuffer->m_size,
                format
            );
        }

        RHI::SingleDeviceStreamBufferView DynamicBufferAllocator::GetStreamBufferView(RHI::Ptr<DynamicBuffer> dynamicBuffer, uint32_t strideByteCount)
        {
            return RHI::SingleDeviceStreamBufferView(
                *m_ringBuffer->GetRHIBuffer(),
                GetBufferAddressOffset(dynamicBuffer),
                dynamicBuffer->m_size,
                strideByteCount
            );
        }

        uint32_t DynamicBufferAllocator::GetBufferAddressOffset(RHI::Ptr<DynamicBuffer> dynamicBuffer)
        {
            return aznumeric_cast<uint32_t>((uint8_t*)dynamicBuffer->m_address - (uint8_t*)m_ringBufferStartAddress);
        }

        void DynamicBufferAllocator::SetEnableAllocationWarning(bool enable)
        {
            m_enableAllocationWarning = enable;
        }

        void DynamicBufferAllocator::FrameEnd()
        {
            uint32_t nextFrame = (m_currentFrame + 1) % AZ::RHI::Limits::Device::FrameCountMax;

            // The saved frame start position will become available since it's old than FrameCountMax. The saved start position of next frame is the new limit
            m_endPositionLimit = m_frameStartPositions[nextFrame];

            // Save start position for current frame
            m_frameStartPositions[m_currentFrame] = m_currentPosition;

            m_currentFrame = nextFrame;

            m_currentAllocatedSize = 0;

        }
    }
}
