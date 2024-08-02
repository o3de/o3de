/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBuffer.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/BufferMemoryView.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
        class BufferPool;
        
        class Buffer final
            : public RHI::DeviceBuffer
        {
            using Base = RHI::DeviceBuffer;
        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::ThreadPoolAllocator);
            AZ_RTTI(Buffer, "{50D79542-AD49-46C8-8660-583A84802105}", Base);
            ~Buffer() = default;
            
            static RHI::Ptr<Buffer> Create();

            // Returns the memory view allocated to this buffer.
            const MemoryView& GetMemoryView() const;
            MemoryView& GetMemoryView();
            
            void SetMapRequestOffset(const uint32_t mapRequestOffset);
            const uint32_t GetMapRequestOffset() const;
            
        private:
            Buffer() = default;
            friend class BufferPool;
            friend class AliasedHeap;
            friend class BufferPoolResolver;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResource
            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const override;
            //////////////////////////////////////////////////////////////////////////
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceBuffer
            using RHI::DeviceBuffer::SetDescriptor;
            //////////////////////////////////////////////////////////////////////////

            BufferMemoryView m_memoryView;
            
            // The number of resolve operations pending for this buffer.
            AZStd::atomic<uint32_t> m_pendingResolves = 0;
            
            // Offset related to the Map request. We need to cache it for cpu/gpu synchronization.
            uint32_t m_mapRequestOffset = 0;

        };
    }
}
