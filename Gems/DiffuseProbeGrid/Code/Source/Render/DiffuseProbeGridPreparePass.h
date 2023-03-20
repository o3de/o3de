/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Render/DiffuseProbeGrid.h>
#include <Render/DiffuseProbeGridFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        //! Compute shader that updates grid data for DiffuseProbeGrids
        class DiffuseProbeGridPreparePass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridPreparePass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridPreparePass, "{5934A485-2550-4100-852C-04A375BCE44E}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridPreparePass, SystemAllocator);
            virtual ~DiffuseProbeGridPreparePass() = default;

            static RPI::Ptr<DiffuseProbeGridPreparePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridPreparePass(const RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // shader
            Data::Instance<RPI::Shader> m_shader;
            const RHI::PipelineState* m_pipelineState = nullptr;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;
            RHI::DispatchDirect m_dispatchArgs;
        };
    }   // namespace Render
}   // namespace AZ
