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

namespace AZ
{
    namespace Render
    {
        //! This pass updates the DiffuseProbeGrid visualization TLAS instances buffer
        class DiffuseProbeGridVisualizationPreparePass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridVisualizationPreparePass);

            AZ_RTTI(DiffuseProbeGridVisualizationPreparePass, "{33BD769D-378B-4142-8C11-6A2ADA2BB095}", Pass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridVisualizationPreparePass, SystemAllocator);

            //! Creates a DiffuseProbeGridVisualizationPreparePass
            static RPI::Ptr<DiffuseProbeGridVisualizationPreparePass> Create(const RPI::PassDescriptor& descriptor);

            ~DiffuseProbeGridVisualizationPreparePass() = default;

        private:
            explicit DiffuseProbeGridVisualizationPreparePass(const RPI::PassDescriptor& descriptor);

            void LoadShader();
            bool ShouldUpdate(const AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid) const;

            // Pass overrides
            bool IsEnabled() const override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // shader
            Data::Instance<RPI::Shader> m_shader;
            const RHI::PipelineState* m_pipelineState = nullptr;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;
            RHI::DispatchDirect m_dispatchArgs;
        };
    }   // namespace RPI
}   // namespace AZ
