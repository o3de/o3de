/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>
#include <Atom/RHI.Reflect/SubpassDependencies.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        //! A RasterPass is a leaf pass (pass with no children) that is used for rasterization.
        class RasterPass
            : public RenderPass
        {
            AZ_RPI_PASS(RasterPass);

        public:
            AZ_RTTI(RasterPass, "{16AF74ED-743C-4842-99F9-347D77BA7F2A}", RenderPass);
            AZ_CLASS_ALLOCATOR(RasterPass, SystemAllocator);
            virtual ~RasterPass();

            //! Creates a RasterPass
            static Ptr<RasterPass> Create(const PassDescriptor& descriptor);

            // Draw List & Pipeline View Tags
            RHI::DrawListTag GetDrawListTag() const override;

            void SetDrawListTag(Name drawListName);

            void SetPipelineStateDataIndex(uint32_t index);

            //! Appends Subpass Attachment Layout data to @subpassLayoutBuilder. Only called when this RasterPass
            //! is a Subpass.
            //! @returns true if the subpass attachment data was appended successfully.
            //! @remarks Invoked by a Parent Pass for each child Raster Pass that should be merged.
            //!     For the most part this is a constant function, except for the fact that @m_subpassIndex
            //!     stores @subpassIndex which will be used later for validation.
            virtual bool BuildSubpassLayout(
                RHI::RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder& subpassLayoutBuilder,
                uint32_t subpassIndex);

            //! Sets the final RenderAttachmentLayout and SubpassDependencies that this RasterPass should use
            //! to work well as a Subpass. Only called when this RasterPass is a Subpass.
            //! @returns true if the incoming data makes sense.
            //! @remarks:
            //!     The data in @renderAttachmentLayout will be used when GetRenderAttachmentConfiguration() is called.
            //!     The data in @subpassDependencies will be used when the FrameGraph compiles the attachment data.
            bool SetRenderAttachmentLayout(const AZStd::shared_ptr<RHI::RenderAttachmentLayout>& renderAttachmentLayout,
                                           const AZStd::shared_ptr<RHI::SubpassDependencies>& subpassDependencies,
                                           uint32_t subpassIndex);

            // RenderPass override
            RHI::RenderAttachmentConfiguration GetRenderAttachmentConfiguration() const override;

            //! Expose shader resource group.
            ShaderResourceGroup* GetShaderResourceGroup();

            uint32_t GetDrawItemCount();

        protected:
            explicit RasterPass(const PassDescriptor& descriptor);

            void DeclareAttachmentsToFrameGraph(RHI::FrameGraphInterface frameGraph) const;

            // Pass behavior overrides
            void Validate(PassValidationResults& validationResults) override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // Retrieve draw lists from view and dynamic draw system and generate final draw list
            void UpdateDrawList();

            // Submit draw items to the context
            virtual void SubmitDrawItems(const RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex, uint32_t indexOffset) const;

            // The draw list tag used to fetch the draw list from the views
            RHI::DrawListTag m_drawListTag;

            // Multiple passes with the same drawListTag can have different pipeline state data (see Scene.h)
            // This is the index of the pipeline state data that corresponds to this pass in the array of pipeline state data
            RHI::Handle<> m_pipelineStateDataIndex;

            // The reference of the draw list to be drawn
            RHI::DrawListView m_drawListView;

            // If there are more than one draw lists from different source: View, DynamicDrawSystem,
            // we need to creates a combined draw list which combines all the draw lists to one and cache it until they are submitted. 
            RHI::DrawList m_combinedDrawList;
            
            // Forces viewport and scissor to match width/height of output image at specified index.
            // Does nothing if index is negative.
            s32 m_viewportAndScissorTargetOutputIndex = -1;

            RHI::Scissor m_scissorState;
            RHI::Viewport m_viewportState;
            bool m_overrideScissorSate = false;
            bool m_overrideViewportState = false;
            uint32_t m_drawItemCount = 0;

            //! The following variables are only relevant when this Raster Pass will be merged by the RHI
            //! as a subpass.
           
            //! Stores the RenderAttachmentLayout that should be used when GetRenderAttachmentConfiguration() is called.
            AZStd::shared_ptr<RHI::RenderAttachmentLayout> m_renderAttachmentLayout;
            //! Stores the custom RHI blob that will be required by the FrameGraph when passes
            //! should me merges as Subpasses/
            AZStd::shared_ptr<RHI::SubpassDependencies> m_subpassDependencies;
            //! Stores the Subpass Index for this subpass.
            uint32_t m_subpassIndex = 0;
        };
    }   // namespace RPI
}   // namespace AZ
