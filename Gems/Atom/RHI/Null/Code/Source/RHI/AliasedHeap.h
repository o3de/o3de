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
    namespace Null
    {
        class AliasedHeap final
            : public RHI::AliasedHeap
        {
            using Base = RHI::AliasedHeap;
        public:
            AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator);
            AZ_RTTI(AliasedHeap, "{F83E0C8A-5421-4E90-9B7A-7FB127B2E7E4}", Base);
            
            static RHI::Ptr<AliasedHeap> Create();                       
        private:
            AliasedHeap() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::AliasedHeap
            AZStd::unique_ptr<RHI::AliasingBarrierTracker> CreateBarrierTrackerInternal() override { return nullptr;}
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::AliasedHeapDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitImageInternal([[maybe_unused]] const RHI::DeviceImageInitRequest& request, [[maybe_unused]] size_t heapOffset) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitBufferInternal([[maybe_unused]] const RHI::DeviceBufferInitRequest& request, [[maybe_unused]] size_t heapOffset) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
