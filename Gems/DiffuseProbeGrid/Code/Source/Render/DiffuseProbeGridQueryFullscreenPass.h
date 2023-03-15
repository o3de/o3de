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
        //! Compute pass that queries the diffuse irradiance from the DiffuseProbeGrids
        class DiffuseProbeGridQueryFullscreenPass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridQueryFullscreenPass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridQueryFullscreenPass, "{DDA729A6-DBC0-4CA0-A0FF-00BC30D1229B}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridQueryFullscreenPass, SystemAllocator, 0);
            virtual ~DiffuseProbeGridQueryFullscreenPass() = default;

            static RPI::Ptr<DiffuseProbeGridQueryFullscreenPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridQueryFullscreenPass(const RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // pass descriptor
            RPI::PassDescriptor m_passDescriptor;

            // shader
            Data::Instance<RPI::Shader> m_shader;
            const RHI::PipelineState* m_pipelineState = nullptr;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_objectSrgLayout;
            RHI::DispatchDirect m_dispatchArgs;

            // output buffer
            RHI::AttachmentId m_outputBufferAttachmentId;
            RHI::BufferDescriptor m_outputBufferDesc;
            RHI::BufferViewDescriptor m_outputBufferViewDesc;
            RPI::Ptr<RPI::PassAttachment> m_outputAttachment;
        };
    }   // namespace Render
}   // namespace AZ
