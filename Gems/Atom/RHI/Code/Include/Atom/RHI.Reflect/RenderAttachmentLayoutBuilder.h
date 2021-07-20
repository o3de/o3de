/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/utils.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    namespace RHI
    {
        //! Provides a convenient way to construct RenderAttachmentLayout objects, which describes
        //! the render attachments layout for the pipeline state.
        //!
        //! The general usage includes adding one or more subpasses, and
        //! adding one or more attachment to each subpass. Examples are shown below.
        //!
        //! 1) One Subpass
        //!   @code
        //!     RHI::RenderAttachmentLayoutBuilder layoutBuilder;
        //!     layoutBuilder.AddSubpas()
        //!         ->RenderTargetAttachment(RHI::Format::R16G16B16_FLOAT)
        //!         ->RenderTargetAttachment(RHI::Format::R8G8B8A8_UNORM)
        //!         ->DepthStencilAttachment(RHI::Format::D32_FLOAT)
        //!     RHI::ResultCode result = layoutBuilder.End(layout);
        //!   @endcode
        //!
        //! 2) Multiple Subpasses
        //!   @code
        //!     RHI::InputStreamLayoutBuilder layoutBuilder;
        //!     layoutBuilder.AddSubpass()
        //!         ->RenderTargetAttachment(RHI::Format::R16G16B16A16_FLOAT, "Color0")
        //!         ->RenderTargetAttachment(RHI::Format::R16G16B16A16_FLOAT, "Color1")
        //!         ->RenderTargetAttachment(RHI::Format::R8G8B8A8_UNORM, "Swapchain")
        //!         ->DepthStencilAttachment(AZ::RHI::Format::D32_FLOAT, "Depth");
        //!     layoutBuilder.AddSubpass()        //!    
        //!         ->RenderTargetAttachment("Swapchain")
        //!         ->DepthStencilAttachment("Depth", AttachmentLoadStoreAction(ClearValue::CreateDepth(1.0f), AttachmentLoadAction::Clear, AttachmentStoreAction::Save));
        //!     RHI::ResultCode result = layoutBuilder.End(layout);
        //!   @endcode
        //!
        //! 3) Multiple Subpasses with subpass inputs
        //!   @code
        //!     RHI::InputStreamLayoutBuilder layoutBuilder;
        //!     layoutBuilder.AddSubpass()
        //!         ->RenderTargetAttachment(RHI::Format::R16G16B16A16_FLOAT, "Color0")
        //!         ->RenderTargetAttachment(RHI::Format::R16G16B16A16_FLOAT, "Color1")
        //!         ->RenderTargetAttachment(RHI::Format::R8G8B8A8_UNORM, "Swapchain")
        //!         ->DepthStencilAttachment(AZ::RHI::Format::D32_FLOAT, "Depth");
        //!     layoutBuilder.AddSubpass()
        //!         ->SubpassInputAttachment("Color0")
        //!         ->RenderTargetAttachment("Color1", AttachmentLoadStoreAction(ClearValue(), AttachmentLoadAction::Load, AttachmentStoreAction::DontCare))
        //!         ->DepthStencilAttachment("Depth");
        //!     layoutBuilder.AddSubpass()
        //!         ->SubpassInputAttachment("Color1")
        //!         ->RenderTargetAttachment("Swapchain")
        //!     RHI::ResultCode result = layoutBuilder.End(layout);
        //!   @endcode
        //!
        class RenderAttachmentLayoutBuilder
        {
        public:
            class SubpassAttachmentLayoutBuilder
            {
                friend class RenderAttachmentLayoutBuilder;
            public:
                SubpassAttachmentLayoutBuilder(uint32_t subpassIndex);

                //! Adds the use of a new render target with resolve information.
                SubpassAttachmentLayoutBuilder* RenderTargetAttachment(
                    Format format,
                    bool resolve);

                //! Adds the use of a previously added render target  with resolve information.
                SubpassAttachmentLayoutBuilder* RenderTargetAttachment(
                    const AZ::Name& name,
                    bool resolve);

                //! Adds the use of a previously added render target.
                SubpassAttachmentLayoutBuilder* RenderTargetAttachment(
                    const AZ::Name& name,
                    const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction(),
                    bool resolve = false);

                //! Adds the use of a new render target.
                SubpassAttachmentLayoutBuilder* RenderTargetAttachment(
                    Format format,                    
                    const AZ::Name& name = {},
                    const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction(),
                    bool resolve = false);

                //! Adds the use of a new resolve attachment. The "sourceName" attachment must
                // be already be added as by this pass.
                SubpassAttachmentLayoutBuilder* ResolveAttachment(
                    const AZ::Name& sourceName,
                    const AZ::Name& resolveName = {});

                //! Adds the use of a new depth/stencil attachment.
                SubpassAttachmentLayoutBuilder* DepthStencilAttachment(
                    Format format,
                    const AZ::Name& name = {},
                    const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction());

                //! Adds the use of a previously added depth/stencil attachment. The "name" attachment must
                // be already be added as by a previous pass.
                SubpassAttachmentLayoutBuilder* DepthStencilAttachment(
                    const AZ::Name name = {},
                    const AttachmentLoadStoreAction& loadStoreAction = AttachmentLoadStoreAction());

                //! Adds the use of a previously added depth/stencil attachment.
                SubpassAttachmentLayoutBuilder* DepthStencilAttachment(
                    const AttachmentLoadStoreAction& loadStoreAction);

                // Adds the use of a subpass input attachment. The "name" attachment must
                // be already be added as by a previous pass.
                SubpassAttachmentLayoutBuilder* SubpassInputAttachment(
                    const AZ::Name& name);

            private:
                struct RenderAttachmentEntry
                {
                    AZ::Name m_name;
                    Format m_format = Format::Unknown;
                    AttachmentLoadStoreAction m_loadStoreAction;
                    AZ::Name m_resolveName;
                };

                AZStd::fixed_vector<RenderAttachmentEntry, RHI::Limits::Pipeline::AttachmentColorCountMax> m_renderTargetAttachments;
                AZStd::fixed_vector<AZ::Name, RHI::Limits::Pipeline::AttachmentColorCountMax> m_subpassInputAttachments;
                RenderAttachmentEntry m_depthStencilAttachment;
                uint32_t m_subpassIndex = 0;
            };

            RenderAttachmentLayoutBuilder();

            //! Adds a new subpass to the layout.
            SubpassAttachmentLayoutBuilder* AddSubpass();

            //! Ends the building of a layout. Returns the result of the operation.
            ResultCode End(RenderAttachmentLayout& builtRenderAttachmentLayout);

            //! Resets all previous values so the builder can be reuse.
            void Reset();

        private:
            /// List of builders for each subpass.
            AZStd::vector<SubpassAttachmentLayoutBuilder> m_subpassLayoutBuilders;
        };
    } // namespace RHI
} // namespace AZ
