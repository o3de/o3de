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
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AZ
{
    namespace Render
    {
        class FullscreenShadowPass final : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(FullscreenShadow);

        public:
            AZ_RTTI(AZ::Render::FullscreenShadowPass, "{A7D3076A-DD01-4B79-AF34-4BB72DAD35E2}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(FullscreenShadowPass, SystemAllocator, 0);
            virtual ~FullscreenShadowPass() = default;

            static RPI::Ptr<FullscreenShadowPass> Create(const RPI::PassDescriptor& descriptor);
            void SetBlendBetweenCascadesEnable(bool enable)
            {
                m_blendBetweenCascadesEnable = enable;
            }

            void SetFilterMethod(AZ::Render::ShadowFilterMethod method)
            {
                m_filterMethod = method;
            }

            void SetReceiverShadowPlaneBiasEnable(bool enable)
            {
                m_receiverShadowPlaneBiasEnable = enable;
            }

            void SetLightIndex(int lightIndex)
            {
                m_lightIndex = lightIndex;
            }

        private:

            bool m_blendBetweenCascadesEnable = false;
            bool m_receiverShadowPlaneBiasEnable = false;
            ShadowFilterMethod m_filterMethod = ShadowFilterMethod::None;

            FullscreenShadowPass(const RPI::PassDescriptor& descriptor);
            AZ::RPI::PassAttachmentBinding GetPassAttachmentBinding(AZ::Name name);

            // Pass behavior overrides...
        //    void BuildInternal() override;

            ///////////////////////////////////////////////////////////////////
            // ShaderReloadNotificationBus overrides...
            //void OnShaderReinitialized(const AZ::RPI::Shader& shader) override;
            //void OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset) override;
            //void OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant& shaderVariant) override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
     //       void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

         //   AZ::RPI::ShaderOptionGroup CreateShaderOptionGroup();

            AZ::RHI::Size GetDepthBufferDimensions();
         //   void ChooseShaderVariant();
         //   const AZ::RPI::ShaderVariant& CreateShaderVariant();
         //   void CreatePipelineStateFromShaderVariant(const RPI::ShaderVariant& shaderVariant);
            void SetConstantData();
         ////   void OnShaderReloaded();

            AZ::RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";

         //   // The shader variant (for choosing the different MSAA version) is sent to the RHI via the PipelineState
         //   AZ::RHI::ConstPtr<AZ::RHI::PipelineState> m_msaaPipelineState;

         //   AZ::Name m_constantDataName;
            int m_lightIndex = 0;
        };
    }   // namespace Render
}   // namespace AZ
