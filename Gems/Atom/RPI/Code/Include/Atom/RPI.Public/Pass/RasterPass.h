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

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        //! A RasterPass is a leaf pass (pass with no children) that is used for rasterization
        //! and it is required to have a valid @m_drawListTag at runtime.
        class ATOM_RPI_PUBLIC_API RasterPass
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

            //! Expose shader resource group.
            ShaderResourceGroup* GetShaderResourceGroup();

            uint32_t GetDrawItemCount();

        protected:
            explicit RasterPass(const PassDescriptor& descriptor);

            // Pass behavior overrides
            void Validate(PassValidationResults& validationResults) override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void InitializeInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // Retrieve draw lists from view and dynamic draw system and generate final draw list
            void UpdateDrawList();

            // Submit draw items to the context
            virtual void SubmitDrawItems(const RHI::FrameGraphExecuteContext& context, uint32_t startIndex, uint32_t endIndex, uint32_t indexOffset) const;

            // Loads the shader resource group of the pass depending on the Supervariant that the pass needs to use.
            void LoadShaderResourceGroup();
        
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

        };
    }   // namespace RPI
}   // namespace AZ
