/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>

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
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridBlendIrradiancePass, SystemAllocator);
            virtual ~DiffuseProbeGridBlendIrradiancePass() = default;

            static RPI::Ptr<DiffuseProbeGridBlendIrradiancePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridBlendIrradiancePass(const RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // shaders
            struct DiffuseProbeGridShader
            {
                Data::Instance<RPI::Shader> m_shader;
                const RHI::PipelineState* m_pipelineState = nullptr;
                RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;
                RHI::DispatchDirect m_dispatchArgs;
            };

            AZStd::vector<DiffuseProbeGridShader> m_shaders;
        };
    }   // namespace Render
}   // namespace AZ
