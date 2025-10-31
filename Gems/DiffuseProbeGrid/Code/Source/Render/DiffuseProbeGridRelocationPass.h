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
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Render/DiffuseProbeGrid.h>

namespace AZ
{
    namespace Render
    {
        //! Compute shader that relocates probes in the diffuse probe grid.
        class DiffuseProbeGridRelocationPass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridRelocationPass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridRelocationPass, "{E6FCBE1A-0404-49B3-AA78-C2B0DEE94FB1}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridRelocationPass, SystemAllocator);
            virtual ~DiffuseProbeGridRelocationPass() = default;

            static RPI::Ptr<DiffuseProbeGridRelocationPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridRelocationPass(const RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;
            void FrameEndInternal() override;

            // shader
            Data::Instance<RPI::Shader> m_shader;
            const RHI::PipelineState* m_pipelineState = nullptr;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_srgLayout;
            RHI::DispatchDirect m_dispatchArgs;

            // revision number of the ray tracing data when the shader table was built
            uint32_t m_rayTracingDataRevision = 0;
        };
    }   // namespace Render
}   // namespace AZ
