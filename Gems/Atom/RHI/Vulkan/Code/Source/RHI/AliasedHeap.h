/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/AliasedHeap.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class Resource;
        class Scope;
        class Buffer;
        class Image;
        class VulkanMemoryAllocation;

        class AliasedHeap final
            : public RHI::AliasedHeap
        {
            using Base = RHI::AliasedHeap;
        public:
            AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator);
            AZ_RTTI(AliasedHeap, "{8C5D201E-2124-4DF3-B645-AF7E699EFD8B}", Base);

            static RHI::Ptr<AliasedHeap> Create();

            struct Descriptor
                : public RHI::AliasedHeapDescriptor
            {
                AZ_CLASS_ALLOCATOR(Descriptor, SystemAllocator)
                VkMemoryRequirements m_memoryRequirements = {};
            };

            const Descriptor& GetDescriptor() const final;

        private:
            AliasedHeap() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::AliasedHeap
            AZStd::unique_ptr<RHI::AliasingBarrierTracker> CreateBarrierTrackerInternal() override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::AliasedHeapDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::DeviceImageInitRequest& request, size_t heapOffset) override;
            RHI::ResultCode InitBufferInternal(const RHI::DeviceBufferInitRequest& request, size_t heapOffset) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::DeviceResource& resource) override;
            //////////////////////////////////////////////////////////////////////////

            Device& GetVulkanRHIDevice() const;

            Descriptor m_descriptor;

            RHI::Ptr<VulkanMemoryAllocation> m_heapMemory;
        };
    }
}
