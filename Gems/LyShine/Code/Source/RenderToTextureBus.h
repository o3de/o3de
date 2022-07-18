/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>

namespace LyShine
{
    //! Ebus to handle render target requests
    class RenderToTextureRequests
        : public AZ::ComponentBus
    {
    public:
        //! Get an attachment image to be used internally by a UI component to render to texture
        //! and then read that same texture (ex. UiMaskComponent, UiFaderComponent)
        virtual AZ::RHI::AttachmentId UseRenderTarget(const AZ::Name& renderTargetName, AZ::RHI::Size size) = 0;

        //! Get an attachment image from an attachment image asset to render to texture only
        //! and then read it outside of LyShine (ex. render UI canvas to a render target and use in a material)
        virtual AZ::RHI::AttachmentId UseRenderTargetAsset(const AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>& attachmentImageAsset) = 0;

        //! Call when a render target is no longer needed by a UI canvas
        virtual void ReleaseRenderTarget(const AZ::RHI::AttachmentId& attachmentId) = 0;

        //! Get an attachment image used by a UI canvas from an attachment image Id
        virtual AZ::Data::Instance<AZ::RPI::AttachmentImage> GetRenderTarget(const AZ::RHI::AttachmentId& attachmentId) = 0;
    };

    using RenderToTextureRequestBus = AZ::EBus<RenderToTextureRequests>;
}
