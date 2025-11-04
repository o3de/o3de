/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! Ray tracing shader that generates the probe visualization image for a DiffuseProbeGrid
        class DiffuseProbeGridVisualizationRayTracingPass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridVisualizationRayTracingPass);

            AZ_RTTI(DiffuseProbeGridVisualizationRayTracingPass, "{CDFB5C03-08D1-4FCA-8B63-2F8326E0DF1D}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridVisualizationRayTracingPass, SystemAllocator);

            //! Creates a DiffuseProbeGridVisualizationRayTracingPass
            static RPI::Ptr<DiffuseProbeGridVisualizationRayTracingPass> Create(const RPI::PassDescriptor& descriptor);

            ~DiffuseProbeGridVisualizationRayTracingPass() = default;

        private:
            explicit DiffuseProbeGridVisualizationRayTracingPass(const RPI::PassDescriptor& descriptor);

            void CreateRayTracingPipelineState();

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // Pass overrides
            bool IsEnabled() const override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // ray tracing shader and pipeline state
            Data::Instance<RPI::Shader> m_rayTracingShader;
            Data::Instance<RPI::Shader> m_missShader;
            Data::Instance<RPI::Shader> m_closestHitShader;
            RHI::Ptr<RHI::RayTracingPipelineState> m_rayTracingPipelineState;

            // ray tracing shader table
            RHI::Ptr<RHI::RayTracingShaderTable> m_rayTracingShaderTable;

            // current size of output attachment, updated every frame
            RHI::Size m_outputAttachmentSize;

            // ray tracing global shader resource group layout and pipeline state
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_globalSrgLayout;
            RHI::ConstPtr<RHI::PipelineState> m_globalPipelineState;

            bool m_initialized = false;
        };
    }   // namespace RPI
}   // namespace AZ
