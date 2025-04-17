/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Reflect/Pass/RenderToTexturePassData.h>

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace RPI
    {
        //! RenderToTexturePass creates a render target and a child pass tree then have the child pass tree render to this render target.
        //! The RenderToTexturePass's pass descriptor defines the render target's size and format and the child pass's template name.
        //! RenderToTexturePass can also read back the render target.
        //! This is useful to render a render pipeline to a render target and (optional) read back its data to cpu memory for later use. 
        class ATOM_RPI_PUBLIC_API RenderToTexturePass final
            : public ParentPass
        {
        public:
            AZ_RTTI(RenderToTexturePass, "{4FBA3461-A072-4538-84D1-311D2756B27E}", ParentPass);
            AZ_CLASS_ALLOCATOR(RenderToTexturePass, SystemAllocator);

            RenderToTexturePass(const PassDescriptor& descriptor);
            ~RenderToTexturePass();

            // Pass class need to have its own Create function for its PassFactory Creator
            static Ptr<RenderToTexturePass> Create(const PassDescriptor& descriptor);

            Ptr<ParentPass> Recreate() const override;
            
            void ReadbackOutput(AZStd::shared_ptr<AttachmentReadback> readback);

            void ResizeOutput(uint32_t width, uint32_t height);

        protected:
            // Pass behavior overrides
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Function to be called when output size changed
            void OnUpdateOutputSize();

        private:
            using Base = ParentPass;

            RHI::Scissor m_scissor;
            RHI::Viewport m_viewport;

            // The child pass used to drive rendering
            Ptr<Pass> m_childPass = nullptr;

            // Name of the template used to create the child pass. Needed for Recreate()
            Name m_childTemplateName;

            Ptr<PassAttachment> m_outputAttachment;

            // saved settings for this pass
            RenderToTexturePassData m_passData;
        };

    }   // namespace RPI
}   // namespace AZ
