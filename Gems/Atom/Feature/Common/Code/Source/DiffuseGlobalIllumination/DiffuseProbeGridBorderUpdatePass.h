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
    namespace Render
    {
        //! Compute shader that updates probe distance and irradiance borders.
        class DiffuseProbeGridBorderUpdatePass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridBorderUpdatePass);

            AZ_RTTI(AZ::Render::DiffuseProbeGridBorderUpdatePass, "{31A5CCD0-CE97-4138-88DA-7BDBD38C9DC8}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridBorderUpdatePass, SystemAllocator, 0);
            virtual ~DiffuseProbeGridBorderUpdatePass() = default;

            static RPI::Ptr<DiffuseProbeGridBorderUpdatePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            DiffuseProbeGridBorderUpdatePass(const RPI::PassDescriptor& descriptor);

            void LoadShader(AZStd::string shaderFilePath,
                            Data::Instance<RPI::Shader>& shader,
                            const RHI::PipelineState*& pipelineState,
                            Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset,
                            RHI::DispatchDirect& dispatchArgs);

            // Pass overrides
            void FrameBeginInternal(FramePrepareParams params) override;

            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // shader
            Data::Instance<RPI::Shader> m_rowShader;
            Data::Instance<RPI::Shader> m_columnShader;
            const RHI::PipelineState* m_rowPipelineState = nullptr;
            const RHI::PipelineState* m_columnPipelineState = nullptr;
            Data::Asset<RPI::ShaderResourceGroupAsset> m_rowSrgAsset;
            Data::Asset<RPI::ShaderResourceGroupAsset> m_columnSrgAsset;
            RHI::DispatchDirect m_rowDispatchArgs;
            RHI::DispatchDirect m_columnDispatchArgs;
        };
    }   // namespace Render
}   // namespace AZ
