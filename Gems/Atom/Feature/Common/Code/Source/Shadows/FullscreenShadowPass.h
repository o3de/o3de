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
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AZ
{
    namespace Render
    {
        class FullscreenShadowPass final
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(FullscreenShadowPass);

            using Base = RPI::FullscreenTrianglePass;

        public:
            AZ_RTTI(AZ::Render::FullscreenShadowPass, "{A7D3076A-DD01-4B79-AF34-4BB72DAD35E2}", RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(FullscreenShadowPass, SystemAllocator);
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

            void SetFilteringSampleCountMode(AZ::Render::ShadowFilterSampleCount filteringSampleCount)
            {
                m_filteringSampleCountMode = filteringSampleCount;
            }

            void SetReceiverShadowPlaneBiasEnable(bool enable)
            {
                m_receiverShadowPlaneBiasEnable = enable;
            }

            // Set directional light's raw index which is used for accessing the directional light in the shader
            void SetLightRawIndex(int lightRawIndex)
            {
                m_lightIndex = lightRawIndex;
            }

        private:

            bool m_blendBetweenCascadesEnable = false;
            bool m_receiverShadowPlaneBiasEnable = false;
            ShadowFilterMethod m_filterMethod = ShadowFilterMethod::None;
            ShadowFilterSampleCount m_filteringSampleCountMode = ShadowFilterSampleCount::PcfTap16;

            FullscreenShadowPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            AZ::RHI::Size GetDepthBufferDimensions();
            uint16_t GetDepthBufferMSAACount();

            void SetConstantData();

            AZ::RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";

            int m_lightIndex = 0;

            AZ::Name m_depthInputName;
            AZ::Name m_outputName;
        };
    }   // namespace Render
}   // namespace AZ
