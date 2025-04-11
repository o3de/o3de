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
            AZ_CLASS_ALLOCATOR(LightCullingTilePreparePass, SystemAllocator);
            virtual ~LightCullingTilePreparePass() = default;

            static RPI::Ptr<LightCullingTilePreparePass> Create(const RPI::PassDescriptor& descriptor);

            static Name GetLightCullingTilePreparePassTemplateName()
            {
                return AZ::Name("LightCullingTilePrepareMSAATemplate");
            }

        private:

            LightCullingTilePreparePass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildInternal() override;

            ///////////////////////////////////////////////////////////////////
            // ShaderReloadNotificationBus overrides...
            void OnShaderReinitialized(const AZ::RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            RHI::MultisampleState GetMultiSampleState();
            AZ::Name GetMultiSampleName();

            AZ::RPI::ShaderOptionGroup CreateShaderOptionGroup();

            AZStd::array<float, 2> ComputeUnprojectConstants() const;
            AZ::RHI::Size GetDepthBufferDimensions();
            void ChooseShaderVariant();
            AZStd::pair<const AZ::RPI::ShaderVariant&, RPI::ShaderOptionGroup> CreateShaderVariant();
            void CreatePipelineStateFromShaderVariant(const RPI::ShaderVariant& shaderVariant, const RPI::ShaderOptionGroup& options);
            void SetConstantData();
            void OnShaderReloaded();

            AZ::RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";

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
