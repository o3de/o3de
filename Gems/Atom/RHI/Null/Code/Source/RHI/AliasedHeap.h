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


namespace AZ
{
    namespace Null
    {
        class AliasedHeap final
            : public RHI::AliasedHeap
        {
            using Base = RHI::AliasedHeap;
        public:
            AZ_CLASS_ALLOCATOR(AliasedHeap, AZ::SystemAllocator, 0);
            AZ_RTTI(AliasedHeap, "{F83E0C8A-5421-4E90-9B7A-7FB127B2E7E4}", Base);
            
            static RHI::Ptr<AliasedHeap> Create();                       
        private:
            AliasedHeap() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::AliasedHeap
            AZStd::unique_ptr<RHI::AliasingBarrierTracker> CreateBarrierTrackerInternal() override { return nullptr;}
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::AliasedHeapDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitImageInternal([[maybe_unused]] const RHI::ImageInitRequest& request, [[maybe_unused]] size_t heapOffset) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitBufferInternal([[maybe_unused]] const RHI::BufferInitRequest& request, [[maybe_unused]] size_t heapOffset) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
