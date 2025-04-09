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
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace Render
    {
        //! Compute shader that updates probe distance and irradiance borders.
        class DiffuseProbeGridBorderUpdatePass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridBorderUpdatePass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridBorderUpdatePass, "{31A5CCD0-CE97-4138-88DA-7BDBD38C9DC8}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridBorderUpdatePass, SystemAllocator);
            virtual ~DiffuseProbeGridBorderUpdatePass() = default;

            static RPI::Ptr<DiffuseProbeGridBorderUpdatePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridBorderUpdatePass(const RPI::PassDescriptor& descriptor);

            void LoadShader(AZStd::string shaderFilePath,
                            Data::Instance<RPI::Shader>& shader,
                            const RHI::PipelineState*& pipelineState,
                            RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout,
                            RHI::DispatchDirect& dispatchArgs);

            // Pass overrides
            bool IsEnabled() const override;
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;
            void FrameEndInternal() override;

            // the data for submits in this pass are pre-built to properly handle submitting on multiple threads
            struct SubmitItem
            {
                RHI::ShaderResourceGroup* m_shaderResourceGroup{nullptr};
                RHI::DispatchItem m_dispatchItem{RHI::MultiDevice::AllDevices};
            };

            AZStd::vector<SubmitItem> m_submitItems;

            // shader
            Data::Instance<RPI::Shader> m_rowShader;
            Data::Instance<RPI::Shader> m_columnShader;
            const RHI::PipelineState* m_rowPipelineState = nullptr;
            const RHI::PipelineState* m_columnPipelineState = nullptr;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_rowSrgLayout;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_columnSrgLayout;
            RHI::DispatchDirect m_rowDispatchArgs;
            RHI::DispatchDirect m_columnDispatchArgs;
        };
    }   // namespace Render
}   // namespace AZ
