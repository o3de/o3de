/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace LyShine
{
    //! Ebus to handle render target requests
    class RenderToTextureRequests
        : public AZ::ComponentBus
    {
    public:
        virtual AZ::RHI::AttachmentId UseRenderTarget(const AZ::Name& renderTargetName, AZ::RHI::Size size) = 0;
        virtual void ReleaseRenderTarget(const AZ::RHI::AttachmentId& attachmentId) = 0;
        virtual AZ::Data::Instance<AZ::RPI::AttachmentImage> GetRenderTarget(const AZ::RHI::AttachmentId& attachmentId) = 0;
    };

    using RenderToTextureRequestBus = AZ::EBus<RenderToTextureRequests>;
}
