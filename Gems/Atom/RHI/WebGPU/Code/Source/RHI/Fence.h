/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceFence.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/optional.h>

namespace AZ::WebGPU
{
    class Fence
        : public RHI::DeviceFence
    {
        using Base = RHI::DeviceFence;
    public:
        AZ_RTTI(Fence, "{41A94F34-1984-4CC3-82BE-02142ECBCA77}", Base);
        AZ_CLASS_ALLOCATOR(Fence, AZ::SystemAllocator);

        static RHI::Ptr<Fence> Create();
        void SignalEvent();
        void WaitEvent() const;
        void SetSignalFuture(const wgpu::Future& future);
            
    private:
        Fence() = default;
            
        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceFence
        RHI::ResultCode InitInternal(RHI::Device& device, RHI::FenceState initialState, bool usedForWaitingOnDevice) override;
        void ShutdownInternal() override;
        void SignalOnCpuInternal() override;
        void WaitOnCpuInternal() const override;
        void ResetInternal() override;
        RHI::FenceState GetFenceStateInternal() const override;
        //////////////////////////////////////////////////////////////////////////

        RHI::FenceState m_state = RHI::FenceState::Reset;
        AZStd::optional<wgpu::Future> m_signalFuture;
        mutable AZStd::condition_variable_any m_eventSignal;
        mutable AZStd::recursive_mutex m_eventMutex;
    };
}
