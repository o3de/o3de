/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRSwapChain.h>

namespace XR
{ 
    SwapChain::View* SwapChain::GetView(AZ::u32 swapchainIndex) const
    {
        return m_viewSwapchains[swapchainIndex].get();
    }

    SwapChain::Image* SwapChain::GetImage(AZ::u32 imageIndex, AZ::u32 swapchainIndex) const
    {
        return GetView(swapchainIndex)->m_images[imageIndex].get();
    }

    AZ::RHI::ResultCode SwapChain::Init(const Descriptor& descriptor)
    {
        m_descriptor = descriptor;
        return InitInternal();
    }
    
    const SwapChain::Descriptor& SwapChain::GetDescriptor() const
    {
        return m_descriptor;
    }

    AZ::u32 SwapChain::GetNumViews() const
    {
        return m_numViews;
    }

    void SwapChain::Shutdown()
    {
        ShutdownInternal();
        m_viewSwapchains.clear();
    }
} // namespace XR
