/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        /*
        * A simple pass to render a texture to an attachment render target
        * The attachment size and format will be configure as the same as the input texture 
        */
        class RenderTexturePass final
            : public RPI::FullscreenTrianglePass
        {
            
            AZ_RPI_PASS(RenderTexturePass);

        public:
            AZ_RTTI(RenderTexturePass, "{476A4E41-08D7-456C-B324-E0493A321FE7}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(RenderTexturePass, SystemAllocator, 0);
            virtual ~RenderTexturePass();

            static RPI::Ptr<RenderTexturePass> Create(const RPI::PassDescriptor& descriptor);

            // Set the source image
            void SetPassSrgImage(AZ::Data::Instance<AZ::RPI::Image> image, RHI::Format format);

            RHI::AttachmentId GetRenderTargetId();

            void InitShaderVariant(bool isNormal);

        protected:
            RenderTexturePass(const RPI::PassDescriptor& descriptor);

        private:

            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void UpdataAttachment();

            RHI::ShaderInputImageIndex m_textureIndex;
            RHI::Size m_attachmentSize;
            RHI::Format m_attachmentFormat;
        };
    }
}
