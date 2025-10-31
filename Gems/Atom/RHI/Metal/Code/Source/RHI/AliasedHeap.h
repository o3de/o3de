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
#include <RHI/Fence.h>
#include <Metal/Metal.h>

namespace AZ
{
    namespace Metal
    {
        class Resource;
        class Scope;

        class AliasedHeap final
            : public RHI::AliasedHeap
        {
            using Base = RHI::AliasedHeap;
        public:
            AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator);
            AZ_RTTI(AliasedHeap, "{C181EDE0-2137-4EF4-B350-7B632B1F2302}", Base);
            
            using Descriptor = RHI::AliasedHeapDescriptor;

            static RHI::Ptr<AliasedHeap> Create();                       

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

            void DeactivateResourceInternal(RHI::DeviceResource& resource, Scope& scope);

            Device& GetMetalRHIDevice() const;        

            /// The resource heap used for allocations.
            id<MTLHeap> m_heap;
        };
    }
}
