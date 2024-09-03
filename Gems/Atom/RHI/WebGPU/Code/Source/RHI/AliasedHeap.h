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
#include <Atom/RHI/AliasingBarrierTracker.h>

namespace AZ::WebGPU
{
    //! No op barrier tracker. Since WebGPU doesn't support aliasing, we don't need to track barriers
    class NoBarrierAliasingBarrierTracker : public RHI::AliasingBarrierTracker
    {
    private:
        //////////////////////////////////////////////////////////////////////////
        // RHI::AliasingBarrierTracker
        void AppendBarrierInternal(
            [[maybe_unused]] const RHI::AliasedResource& resourceBefore,
            [[maybe_unused]] const RHI::AliasedResource& resourceAfter) override
        {
        }
        //////////////////////////////////////////////////////////////////////////
    };

    //! Since aliasing is not supported for WebGPU, this heap doesn't really share any memory
    //! between the resources. Each resource has it's own memory, managed by the wgpu resource.
    class AliasedHeap final
        : public RHI::AliasedHeap
    {
        using Base = RHI::AliasedHeap;
    public:
        AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator);
        AZ_RTTI(AliasedHeap, "{D832F0CA-C298-4048-B753-9FE42E22EA7E}", Base);
            
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
    };
}
