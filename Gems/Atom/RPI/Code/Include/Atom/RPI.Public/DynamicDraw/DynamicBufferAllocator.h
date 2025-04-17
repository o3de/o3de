/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RPI.Public/Buffer/RingBuffer.h>
#include <Atom/RPI.Public/Configuration.h>

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
        class ATOM_RPI_PUBLIC_API DynamicBufferAllocator
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
            //! It may return nullptr if the input size is larger than ring buffer size or there isn't enough unused memory available within
            //! the ring buffer
            RHI::Ptr<DynamicBuffer> Allocate(uint32_t size, uint32_t alignment);

            //! Get an IndexBufferView for a DynamicBuffer used as an index buffer
            RHI::IndexBufferView GetIndexBufferView(RHI::Ptr<DynamicBuffer> subBuffer, RHI::IndexFormat format);

            //! Get an StreamBufferView for a DynamicBuffer used as a vertex buffer
            RHI::StreamBufferView GetStreamBufferView(RHI::Ptr<DynamicBuffer> dynamicBuffer, uint32_t strideByteCount);

            //! Submit allocated dynamic buffer to gpu for current frame
            void FrameEnd();

            //! Enable/disable buffer allocation warning if allocation fails
            void SetEnableAllocationWarning(bool enable);

        private:
            // Get buffer's offset;
            uint32_t GetBufferAddressOffset(RHI::Ptr<DynamicBuffer> dynamicBuffer);

            // The position where the buffer is available.
            uint32_t m_currentPosition = 0;

            // The size of the buffer per frame
            uint32_t m_ringBufferSize = 0;

            bool m_enableAllocationWarning = false;

            // The resident buffer data per frame
            RingBuffer m_bufferData{ "DynamicBufferAllocator", CommonBufferPoolType::DynamicInputAssembly, 1 };

            // The CPU address of the mapped buffer per frame
            RHI::FrameCountMaxRingBuffer<AZStd::unordered_map<int, void*>> m_bufferStartAddresses;
        };
    } // namespace RPI
} // namespace AZ
