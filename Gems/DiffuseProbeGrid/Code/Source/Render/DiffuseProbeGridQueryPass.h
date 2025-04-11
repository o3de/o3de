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
        class DiffuseProbeGridQueryPass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridQueryPass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridQueryPass, "{EE52C193-98E2-4F93-8A19-B4E710F12592}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridQueryPass, SystemAllocator);
            virtual ~DiffuseProbeGridQueryPass() = default;

            static RPI::Ptr<DiffuseProbeGridQueryPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridQueryPass(const RPI::PassDescriptor& descriptor);

            void LoadShader();

            // Pass overrides
            bool IsEnabled() const override;
            void BuildInternal() override;
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

            // output buffer
            RHI::AttachmentId m_outputBufferAttachmentId;
            RHI::BufferDescriptor m_outputBufferDesc;
            RHI::BufferViewDescriptor m_outputBufferViewDesc;
            RPI::Ptr<RPI::PassAttachment> m_outputAttachment;
        };
    }   // namespace Render
}   // namespace AZ
