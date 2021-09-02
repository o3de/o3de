/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Fence.h>

namespace AZ
{
    namespace Null
    {
        class Fence
            : public RHI::Fence
        {
            using Base = RHI::Fence;
        public:
            AZ_RTTI(Fence, "{34908F40-A7DE-4EE8-A871-71ACE0C24972}", Base);
            AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator, 0);

            static RHI::Ptr<Fence> Create();
            
        private:
            Fence() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::Fence
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] RHI::FenceState initialState) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            void SignalOnCpuInternal() override {}
            void WaitOnCpuInternal() const override {}
            void ResetInternal() override {}
            RHI::FenceState GetFenceStateInternal() const override { return RHI::FenceState::Signaled;};
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
