/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Fence.h>
#include <RHI/Instance.h>

namespace AZ::WebGPU
{
    RHI::Ptr<Fence> Fence::Create()
    {
        return aznew Fence();
    }

    RHI::ResultCode Fence::InitInternal([[maybe_unused]] RHI::Device& device, RHI::FenceState initialState, [[maybe_unused]] bool usedForWaitingOnDevice)
    {
        m_state = initialState;
        AZ_Assert(!usedForWaitingOnDevice, "Waiting on Fences on the GPU is not supported on WebGPU.");
        return usedForWaitingOnDevice ? RHI::ResultCode::InvalidArgument : RHI::ResultCode::Success;
    }

    void Fence::ShutdownInternal()
    {
    }

    void Fence::SignalOnCpuInternal()
    {
        AZ_Error("WebGPU", false, "SignalOnCpuInternal is not supported on WebGPU");
    }

    void Fence::WaitOnCpuInternal() const
    {
        WaitEvent();
    }

    void Fence::ResetInternal()
    {
        AZ_Assert(!m_signalFuture.has_value(), "Cannot reset fence when it has a signal future pending");
        m_state = RHI::FenceState::Reset;
    }

    RHI::FenceState Fence::GetFenceStateInternal() const
    {
        return m_state;
    }

    void Fence::SignalEvent()
    {
        m_state = RHI::FenceState::Signaled;
        m_signalFuture = {};
    }

    void Fence::WaitEvent() const
    {
        if (m_state != RHI::FenceState::Signaled)
        {
            AZ_Assert(m_signalFuture.has_value(), "Trying to wait on event before signal future has been set");
            wgpu::Instance& instance = Instance::GetInstance().GetNativeInstance();
            wgpu::WaitStatus status = instance.WaitAny(m_signalFuture.value(), UINT64_MAX);
            AZ_Error("WebGPU", status == wgpu::WaitStatus::Success, "WaitAny error %d when doing Fence::WaitEvent", status);
        }
    }

    void Fence::SetSignalFuture(const wgpu::Future& future)
    {
        Reset();
        m_signalFuture = future;
    }
}
