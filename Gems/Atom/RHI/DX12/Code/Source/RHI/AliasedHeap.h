/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/AliasedHeap.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <RHI/DX12.h>

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
            AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator);
            AZ_RTTI(AliasedHeap, "{EE67B349-67EC-40BC-8E57-94FD6338C143}", Base);

            static RHI::Ptr<AliasedHeap> Create();

            struct Descriptor
                : public RHI::AliasedHeapDescriptor
            {
                AZ_CLASS_ALLOCATOR(Descriptor, SystemAllocator)
                D3D12_HEAP_FLAGS m_heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
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

            Device& GetDX12RHIDevice();

            Descriptor m_descriptor;

            /// The resource heap used for allocations.
            RHI::Ptr<ID3D12Heap> m_heap;
        };
    }
}
