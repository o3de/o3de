/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

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
            AZ_CLASS_ALLOCATOR(RasterPass, SystemAllocator, 0);
            virtual ~RasterPass();

            //! Creates a RasterPass
            static Ptr<RasterPass> Create(const PassDescriptor& descriptor);

            // Draw List & Pipeline View Tags
            RHI::DrawListTag GetDrawListTag() const override;

            void SetDrawListTag(Name drawListName);

            void SetPipelineStateDataIndex(u32 index);

            //! Expose shader resource group.
            ShaderResourceGroup* GetShaderResourceGroup();

        protected:
            explicit RasterPass(const PassDescriptor& descriptor);

            // Pass behavior overrides
            void Validate(PassValidationResults& validationResults) override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // The draw list tag used to fetch the draw list from the views
            RHI::DrawListTag m_drawListTag;

            // Multiple passes with the same drawListTag can have different pipeline state data (see Scene.h)
            // This is the index of the pipeline state data that corresponds to this pass in the array of pipeline state data
            RHI::Handle<> m_pipelineStateDataIndex;

            // The draw list returned from the view
            RHI::DrawListView m_drawListView;
            
            RHI::Scissor m_scissorState;
            RHI::Viewport m_viewportState;
            bool m_overrideScissorSate = false;
            bool m_overrideViewportState = false;
        };
    }   // namespace RPI
}   // namespace AZ
