/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRDevice.h>
#include <XR/XRSwapChain.h>

namespace XR
{
    AZ::RHI::ResultCode Device::Init(Descriptor descriptor)
    {
        m_descriptor = descriptor;
        return AZ::RHI::ResultCode::Success;
    }
    
    const Device::Descriptor& Device::GetDescriptor() const
    {
        return m_descriptor;
    }

    Ptr<Session> Device::GetSession() const
    {
        return m_session;
    }

    bool Device::BeginFrame()
    {
        return BeginFrameInternal();
    }

    void Device::EndFrame(Ptr<SwapChain> swapChain)
    {
        EndFrameInternal(swapChain);
    }

    void Device::PostFrame()
    {
        PostFrameInternal();
    }

    bool Device::AcquireSwapChainImage(AZ::u32 viewIndex, XR::SwapChain* baseSwapChain)
    {
        return AcquireSwapChainImageInternal(viewIndex, baseSwapChain);
    }

    void Device::RegisterSession(Ptr<Session> session)
    {
        m_session = session;
    }

    void Device::UnRegisterSession()
    {
        m_session = nullptr;
    }

    void Device::Shutdown()
    {
        ShutdownInternal();
    }

} // namespace XR
