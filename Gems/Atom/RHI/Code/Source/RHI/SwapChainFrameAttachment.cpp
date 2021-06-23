/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom/RHI/SwapChain.h>

namespace AZ
{
    namespace RHI
    {
        SwapChainFrameAttachment::SwapChainFrameAttachment(
            const AttachmentId& attachmentId,
            Ptr<SwapChain> swapChain)
            : ImageFrameAttachment(attachmentId, swapChain->GetCurrentImage())
            , m_swapChain{AZStd::move(swapChain)}
        {}

        SwapChain* SwapChainFrameAttachment::GetSwapChain()
        {
            return m_swapChain.get();
        }

        const SwapChain* SwapChainFrameAttachment::GetSwapChain() const
        {
            return m_swapChain.get();
        }
    }
}
