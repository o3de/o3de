/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Fence.h>

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
        m_eventSignal.notify_all();
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
        SignalEvent();
        m_state = RHI::FenceState::Reset;
    }

    RHI::FenceState Fence::GetFenceStateInternal() const
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_eventMutex);
        return m_state;
    }

    void Fence::SignalEvent()
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_eventMutex);
        m_state = RHI::FenceState::Signaled;
        m_eventSignal.notify_all();
    }

    void Fence::WaitEvent() const
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_eventMutex);
        if (m_state != RHI::FenceState::Signaled)
        {
            m_eventSignal.wait(
                lock,
                [&]()
                {
                    return m_state != RHI::FenceState::Signaled;
                });
        }       
    }
}
