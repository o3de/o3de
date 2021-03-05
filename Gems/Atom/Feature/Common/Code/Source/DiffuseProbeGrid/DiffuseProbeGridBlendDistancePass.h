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
        //! Compute shader that generates the distance texture from the diffuse probe raytrace.
        class DiffuseProbeGridBlendDistancePass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridBlendDistancePass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridBlendDistancePass, "{8D57B51E-3432-49F6-83F8-DB6DF2F5A5CA}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridBlendDistancePass, SystemAllocator, 0);
            virtual ~DiffuseProbeGridBlendDistancePass() = default;

            static RPI::Ptr<DiffuseProbeGridBlendDistancePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridBlendDistancePass(const RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            void FrameBeginInternal(FramePrepareParams params) override;

            virtual void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer) override;
            virtual void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;
            virtual void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

            // shader
            Data::Instance<RPI::Shader> m_shader;
            const RHI::PipelineState* m_pipelineState = nullptr;
            Data::Asset<RPI::ShaderResourceGroupAsset> m_srgAsset;
            RHI::DispatchDirect m_dispatchArgs;
        };
    }   // namespace Render
}   // namespace AZ
