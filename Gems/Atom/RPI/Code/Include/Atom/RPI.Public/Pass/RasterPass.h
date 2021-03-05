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

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const PassScopeProducer& producer) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const PassScopeProducer& producer) override;

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
