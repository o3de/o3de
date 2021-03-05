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
#include <AzCore/std/containers/unordered_map.h>

#include <AzFramework/Windowing/WindowBus.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace AZ
{
    namespace Render
    {
        static const char* const LookModificationTransformPassTemplateName{ "LookModificationTransformTemplate" };
        static const char* const ExposureShaderVariantOptionName{ "o_enableExposureControlFeature" };
        static const char* const ColorGradingShaderVariantOptionName{ "o_enableColorGradingLut" };

        /**
         *  The look modification composite pass. If color grading LUTs are enabled, this pass will apply the blended LUT.
         */
        class LookModificationCompositePass
            : public AZ::RPI::FullscreenTrianglePass
            , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(LookModificationCompositePass, "{D7DF3E8A-B642-4D51-ABC2-ADB2B60FCE1D}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(LookModificationCompositePass, SystemAllocator, 0);
            virtual ~LookModificationCompositePass() = default;

            //! Creates a LookModificationPass
            static RPI::Ptr<LookModificationCompositePass> Create(const RPI::PassDescriptor& descriptor);

            //! Set whether exposure control is enabled
            void SetExposureControlEnabled(bool enabled);

            //! Set shaper parameters
            void SetShaperParameters(const ShaperParams& shaperParams);

        protected:
            LookModificationCompositePass(const RPI::PassDescriptor& descriptor);
            void Init() override;
            
            //! Pass behavior overrides
            void FrameBeginInternal(FramePrepareParams params) final;

            RHI::ShaderInputConstantIndex   m_exposureControlEnabledFlagIndex;

        private:
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            //! Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

            void UpdateExposureFeatureState();
            void UpdateLookModificationFeatureState();

            void SetColorGradingLutEnabled(bool enabled);

            bool m_exposureControlEnabled = false;
            bool m_colorGradingLutEnabled = false;
            Render::DisplayMapperLut m_blendedColorGradingLut;
            Render::ShaperParams m_colorGradingShaperParams;

            const AZ::Name m_exposureShaderVariantOptionName;
            const AZ::Name m_colorGradingShaderVariantOptionName;
            bool m_needToUpdateShaderVariant = true;

            RHI::ShaderInputImageIndex m_shaderColorGradingLutImageIndex;

            RHI::ShaderInputConstantIndex m_shaderColorGradingShaperTypeIndex;
            RHI::ShaderInputConstantIndex m_shaderColorGradingShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderColorGradingShaperScaleIndex;
        };
    }   // namespace Render
}   // namespace AZ
