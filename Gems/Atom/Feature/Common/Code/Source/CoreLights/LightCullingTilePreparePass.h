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
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! LightCullingTilePrepare
        //! This pass analyzes the depth buffer and builds min/max depth bounds per each tile.
        class LightCullingTilePreparePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(LightCullingTilePreparePass);

        public:
            AZ_RTTI(AZ::Render::LightCullingTilePreparePass, "{3D8AEEF2-F679-4C0C-ADF0-2F7E347D8BF1}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(LightCullingTilePreparePass, SystemAllocator, 0);
            virtual ~LightCullingTilePreparePass() = default;

            static RPI::Ptr<LightCullingTilePreparePass> Create(const RPI::PassDescriptor& descriptor);

            static Name GetLightCullingTilePreparePassTemplateName()
            {
                return AZ::Name("LightCullingTilePrepareTemplate");
            }

        private:

            LightCullingTilePreparePass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildAttachmentsInternal() override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

            RHI::MultisampleState GetMultiSampleState();
            AZ::Name GetMultiSampleName();

            AZ::RPI::ShaderOptionGroup CreateShaderOptionGroup();

            AZStd::array<float, 2> ComputeUnprojectConstants() const;
            AZ::RHI::Size GetDepthBufferDimensions();
            void ChooseShaderVariant();
            const AZ::RPI::ShaderVariant& CreateShaderVariant();
            void CreatePipelineStateFromShaderVariant(const RPI::ShaderVariant& shaderVariant);
            void SetConstantData();
            AZ::RHI::ShaderInputConstantIndex m_constantDataIndex;

            // The shader variant (for choosing the different MSAA version) is sent to the RHI via the PipelineState
            AZ::RHI::ConstPtr<AZ::RHI::PipelineState> m_msaaPipelineState;

            AZ::Name m_msaaNoneName;
            AZ::Name m_msaaMode2xName;
            AZ::Name m_msaaMode4xName;
            AZ::Name m_msaaMode8xName;
            AZ::Name m_msaaOptionName;
            AZ::Name m_constantDataName;
        };
    }   // namespace Render
}   // namespace AZ
