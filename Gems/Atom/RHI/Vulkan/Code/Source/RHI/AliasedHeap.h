/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/AliasedHeap.h>
#include <RHI/Memory.h>

namespace AZ
{
    namespace Vulkan
    {
        class Resource;
        class Scope;
        class Buffer;
        class Image;

        class AliasedHeap final
            : public RHI::AliasedHeap
        {
            using Base = RHI::AliasedHeap;
        public:
            AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator, 0);
            AZ_RTTI(AliasedHeap, "{8C5D201E-2124-4DF3-B645-AF7E699EFD8B}", Base);

            static RHI::Ptr<AliasedHeap> Create();

            struct Descriptor
                : public RHI::AliasedHeapDescriptor
            {
                uint32_t m_memoryTypeMask = 0;
                VkMemoryPropertyFlags m_memoryFlags = 0;
            };            

            const Descriptor& GetDescriptor() const final;

        private:
            AliasedHeap() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::AliasedHeap
            AZStd::unique_ptr<RHI::AliasingBarrierTracker> CreateBarrierTrackerInternal() override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::AliasedHeapDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::ImageInitRequest& request, size_t heapOffset) override;
            RHI::ResultCode InitBufferInternal(const RHI::BufferInitRequest& request, size_t heapOffset) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::ResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::Resource& resource) override;
            //////////////////////////////////////////////////////////////////////////

            Device& GetVulkanRHIDevice() const;

            Descriptor m_descriptor;

            RHI::Ptr<Memory> m_heapMemory;
        };
    }
}
