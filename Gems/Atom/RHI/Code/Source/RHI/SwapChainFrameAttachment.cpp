/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom/RHI/SingleDeviceSwapChain.h>

namespace AZ::RHI
{
    SwapChainFrameAttachment::SwapChainFrameAttachment(
        const AttachmentId& attachmentId,
        Ptr<SingleDeviceSwapChain> swapChain)
        : ImageFrameAttachment(attachmentId, swapChain->GetCurrentImage())
        , m_swapChain{AZStd::move(swapChain)}
    {}

    SingleDeviceSwapChain* SwapChainFrameAttachment::GetSwapChain()
    {
        return m_swapChain.get();
    }

    const SingleDeviceSwapChain* SwapChainFrameAttachment::GetSwapChain() const
    {
        return m_swapChain.get();
    }
}
