/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceFence.h>

namespace AZ
{
    namespace WebGPU
    {
        class Fence
            : public RHI::DeviceFence
        {
            using Base = RHI::DeviceFence;
        public:
            AZ_RTTI(Fence, "{41A94F34-1984-4CC3-82BE-02142ECBCA77}", Base);
            AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator);

            static RHI::Ptr<Fence> Create();
            
        private:
            Fence() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceFence
            RHI::ResultCode InitInternal(
                [[maybe_unused]] RHI::Device& device,
                [[maybe_unused]] RHI::FenceState initialState,
                [[maybe_unused]] bool usedForWaitingOnDevice) override
            {
                return RHI::ResultCode::Success;
            }
            void ShutdownInternal() override {}
            void SignalOnCpuInternal() override {}
            void WaitOnCpuInternal() const override {}
            void ResetInternal() override {}
            RHI::FenceState GetFenceStateInternal() const override { return RHI::FenceState::Signaled;};
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
