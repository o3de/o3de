/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AZ
{
    namespace Render
    {
        class FullscreenShadowComputePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(FullscreenShadowComputePass);

            using Base = RPI::ComputePass;

        public:
            AZ_RTTI(AZ::Render::FullscreenShadowComputePass, "{e96f2a93-0c8e-4086-9b71-068bc92aa09c}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(FullscreenShadowComputePass, SystemAllocator, 0);
            virtual ~FullscreenShadowComputePass() = default;
            //! Creates an FullscreenShadowComputePass
            static RPI::Ptr<FullscreenShadowComputePass> Create(const RPI::PassDescriptor& descriptor);
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
        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            FullscreenShadowComputePass(const RPI::PassDescriptor& descriptor);

            AZ::RHI::Size GetDepthBufferDimensions();

            int GetDepthBufferMSAACount();

            void SetConstantData();

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantDataIndex = "m_constantData";
            FullScreenShadowConstantData constantData;
            
            bool m_blendBetweenCascadesEnable = false;
            bool m_receiverShadowPlaneBiasEnable = false;
            ShadowFilterMethod m_filterMethod = ShadowFilterMethod::None;

            int m_lightIndex = 0;

            AZ::Name m_depthInputName;
            AZ::Name m_outputName;
        };
    }   // namespace Render
}   // namespace AZ
