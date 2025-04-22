/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ImageFrameAttachment.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ::RHI
{
    class SwapChain;

    //! A swap chain registered into the frame scheduler.
    class SwapChainFrameAttachment final
        : public ImageFrameAttachment
    {
    public:
        AZ_RTTI(SwapChainFrameAttachment, "{6DBAE3A9-45F9-4B0A-AFF4-0965C456D4C0}", ImageFrameAttachment);
        AZ_CLASS_ALLOCATOR(SwapChainFrameAttachment, AZ::PoolAllocator);

        SwapChainFrameAttachment(
            const AttachmentId& attachmentId,
            Ptr<SwapChain> swapChain);

        /// Returns the swap chain referenced by this attachment.
        const SwapChain* GetSwapChain() const;
        SwapChain* GetSwapChain();

    private:
        Ptr<SwapChain> m_swapChain;
    };
}
