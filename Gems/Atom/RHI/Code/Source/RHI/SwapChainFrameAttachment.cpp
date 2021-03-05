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