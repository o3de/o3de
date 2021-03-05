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

#include <Atom/RHI/AliasedHeap.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class Resource;
        class Scope;

        class AliasedHeap final
            : public RHI::AliasedHeap
        {
            using Base = RHI::AliasedHeap;
        public:
            AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator, 0);
            AZ_RTTI(AliasedHeap, "{EE67B349-67EC-40BC-8E57-94FD6338C143}", Base);

            static RHI::Ptr<AliasedHeap> Create();

            struct Descriptor
                : public RHI::AliasedHeapDescriptor
            {
                D3D12_HEAP_FLAGS m_heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
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

            Device& GetDX12RHIDevice();

            Descriptor m_descriptor;

            /// The resource heap used for allocations.
            RHI::Ptr<ID3D12Heap> m_heap;
        };
    }
}
