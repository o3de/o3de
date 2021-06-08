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

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGrid.h>

namespace AZ
{
    namespace Render
    {
        //! Compute shader that classifies probes as active or inactive in the diffuse probe grid.
        class DiffuseProbeGridClassificationPass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridClassificationPass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridClassificationPass, "{98A6477A-F31C-4390-9BEB-9DB8E30BB281}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridClassificationPass, SystemAllocator, 0);
            virtual ~DiffuseProbeGridClassificationPass() = default;

            static RPI::Ptr<DiffuseProbeGridClassificationPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridClassificationPass(const RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            void FrameBeginInternal(FramePrepareParams params) override;

            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // shader
            Data::Instance<RPI::Shader> m_shader;
            const RHI::PipelineState* m_pipelineState = nullptr;
            Data::Asset<RPI::ShaderResourceGroupAsset> m_srgAsset;
            RHI::DispatchDirect m_dispatchArgs;
        };
    }   // namespace Render
}   // namespace AZ
