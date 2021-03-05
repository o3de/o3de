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
            AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator, 0);
            AZ_RTTI(AliasedHeap, "{C181EDE0-2137-4EF4-B350-7B632B1F2302}", Base);
            
            using Descriptor = RHI::AliasedHeapDescriptor;

            static RHI::Ptr<AliasedHeap> Create();                       

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

            void DeactivateResourceInternal(RHI::Resource& resource, Scope& scope);

            Device& GetMetalRHIDevice() const;        

            /// The resource heap used for allocations.
            id<MTLHeap> m_heap;
        };
    }
}
