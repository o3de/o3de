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
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace RHI
    {
        class PipelineState;
    }

    namespace Render
    {
        //! Compute shader that generates the irradiance texture from the diffuse probe raytrace.
        class DiffuseProbeGridBlendIrradiancePass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridBlendIrradiancePass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridBlendIrradiancePass, "{521E53EE-2D12-454F-8830-C61EA08CEF35}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridBlendIrradiancePass, SystemAllocator, 0);
            virtual ~DiffuseProbeGridBlendIrradiancePass() = default;

            static RPI::Ptr<DiffuseProbeGridBlendIrradiancePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridBlendIrradiancePass(const RPI::PassDescriptor& descriptor);

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
