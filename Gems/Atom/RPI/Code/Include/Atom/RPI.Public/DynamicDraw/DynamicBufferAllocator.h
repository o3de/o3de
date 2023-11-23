/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomCore/Instance/Instance.h>

#include <Atom/RHI/SingleDeviceIndexBufferView.h>
#include <Atom/RHI/SingleDeviceStreamBufferView.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>


namespace AZ
{
    namespace RPI
    {
        class DynamicBuffer;

        //! DynamicBufferAllocator allocates DynamicBuffers within a big pre-allocated buffer by using ring buffer allocation
        //! The addresses of allocated DynamicBuffers would be available after AZ::RHI::Limits::Device::FrameCountMax frames.
        //! Since the allocations are sub-allocations they almost have zero cost with both cpu and gpu.
        //! Limitation: the allocation may fail if the request buffer size is larger than the ring buffer size or
        //!     there isn't enough unused memory available within the ring buffer. User may increase the input of Init(ringBufferSize)
        //!     to increase the ring buffer's size. 
        class DynamicBufferAllocator
        {
        public:
            AZ_RTTI(AZ::RPI::DynamicBufferAllocator, "{82B047B3-C845-4F77-9852-747E39C53081}");

            DynamicBufferAllocator() = default;
            virtual ~DynamicBufferAllocator() = default;

            //! One time initialization
            //! This operation may be slow since it will allocate large size gpu resource. 
            void Init(uint32_t ringBufferSize);

            void Shutdown();

            //! Allocate a dynamic buffer with specified size and alignment
            //! It may return nullptr if the input size is larger than ring buffer size or there isn't enough unused memory available within the ring buffer
            RHI::Ptr<DynamicBuffer> Allocate(uint32_t size, uint32_t alignment);

            //! Get an IndexBufferView for a DynamicBuffer used as an index buffer
            RHI::SingleDeviceIndexBufferView GetIndexBufferView(RHI::Ptr<DynamicBuffer> subBuffer, RHI::IndexFormat format);

            //! Get an StreamBufferView for a DynamicBuffer used as a vertex buffer
            RHI::SingleDeviceStreamBufferView GetStreamBufferView(RHI::Ptr<DynamicBuffer> dynamicBuffer, uint32_t strideByteCount);

            //! Submit allocated dynamic buffer to gpu for current frame
            void FrameEnd();

            //! Enable/disable buffer allocation warning if allocation fails
            void SetEnableAllocationWarning(bool enable);

        private:
            // Get buffer's offset;
            uint32_t GetBufferAddressOffset(RHI::Ptr<DynamicBuffer> dynamicBuffer);

            // The position where the buffer is available.
            uint32_t m_currentPosition = 0;
            // The upper bound limit of the allocation of current frame 
            uint32_t m_endPositionLimit = 0;
            // The allocated buffer size of current frame
            uint32_t m_currentAllocatedSize = 0;

            uint32_t m_ringBufferSize = 0;
            void* m_ringBufferStartAddress = 0;
            Data::Instance<Buffer> m_ringBuffer;

            // Allocation history which are in use by GPU. 
            uint32_t m_frameStartPositions[AZ::RHI::Limits::Device::FrameCountMax];
            uint32_t m_currentFrame = 0;

            bool m_enableAllocationWarning = false;
        };
    }
}
