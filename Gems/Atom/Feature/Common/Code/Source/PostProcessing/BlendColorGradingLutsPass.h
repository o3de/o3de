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

#include <Atom/RHI/ImagePool.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <PostProcess/LookModification/LookModificationSettings.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace Render
    {

        //! This pass will blend together multiple color blending LUTs based on their individual intensity setting as well as their
        //! override strengths.
        //! If there is only one LUT, blending will happen with the LUT and the reference non color graded values based on that LUT's intensity value.
        class BlendColorGradingLutsPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(BlendColorGradingLutsPass);

        public:
            AZ_RTTI(BlendColorGradingLutsPass, "{F1E7ED65-27B1-4AF3-AF8D-C29C2BF31EE7}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(BlendColorGradingLutsPass, SystemAllocator);
            virtual ~BlendColorGradingLutsPass();

            /// Creates a DisplayMapperPass
            static RPI::Ptr<BlendColorGradingLutsPass> Create(const RPI::PassDescriptor& descriptor);

            void SetShaperParameters(const ShaperParams& shaperParams);
            AZStd::optional<ShaperParams> GetCommonShaperParams() const;

        private:
            explicit BlendColorGradingLutsPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void InitializeInternal() override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            void AcquireLutImage();
            void ReleaseLutImage();

            void CheckLutBlendSettings();
            LookModificationSettings* GetLookModificationSettings() const;

            bool m_resourcesInitialized = false;

            const AZ::Name m_numSourceLutsShaderVariantOptionName;
            bool m_needToUpdateShaderVariant = false;

            RHI::ShaderInputNameIndex m_shaderInputBlendedLutImageIndex = "m_blendedLut";
            RHI::ShaderInputNameIndex m_shaderInputBlendedLutDimensionsIndex = "m_blendedLutDimensions";
            RHI::ShaderInputNameIndex m_shaderInputBlendedLutShaperTypeIndex = "m_blendedLutShaperType";
            RHI::ShaderInputNameIndex m_shaderInputBlendededLutShaperBiasIndex = "m_blendedLutShaperBias";
            RHI::ShaderInputNameIndex m_shaderInputBlendededLutShaperScaleIndex = "m_blendedLutShaperScale";

            RHI::ShaderInputNameIndex m_shaderInputSourceLut1ImageIndex = "m_sourceLut1";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut1ShaperTypeIndex = "m_sourceLut1ShaperType";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut1ShaperBiasIndex = "m_sourceLut1ShaperBias";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut1ShaperScaleIndex = "m_sourceLut1ShaperScale";

            RHI::ShaderInputNameIndex m_shaderInputSourceLut2ImageIndex = "m_sourceLut2";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut2ShaperTypeIndex = "m_sourceLut2ShaperType";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut2ShaperBiasIndex = "m_sourceLut2ShaperBias";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut2ShaperScaleIndex = "m_sourceLut2ShaperScale";

            RHI::ShaderInputNameIndex m_shaderInputSourceLut3ImageIndex = "m_sourceLut3";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut3ShaperTypeIndex = "m_sourceLut3ShaperType";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut3ShaperBiasIndex = "m_sourceLut3ShaperBias";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut3ShaperScaleIndex = "m_sourceLut3ShaperScale";

            RHI::ShaderInputNameIndex m_shaderInputSourceLut4ImageIndex = "m_sourceLut4";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut4ShaperTypeIndex = "m_sourceLut4ShaperType";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut4ShaperBiasIndex = "m_sourceLut4ShaperBias";
            RHI::ShaderInputNameIndex m_shaderInputSourceLut4ShaperScaleIndex = "m_sourceLut4ShaperScale";

            RHI::ShaderInputNameIndex m_shaderInputWeight0Index = "m_weight0";
            RHI::ShaderInputNameIndex m_shaderInputWeight1Index = "m_weight1";
            RHI::ShaderInputNameIndex m_shaderInputWeight2Index = "m_weight2";
            RHI::ShaderInputNameIndex m_shaderInputWeight3Index = "m_weight3";
            RHI::ShaderInputNameIndex m_shaderInputWeight4Index = "m_weight4";

            AZ::Render::DisplayMapperParameters m_displayMapperParameters = {};

            DisplayMapperLut m_blendedLut = {};
            ShaperParams m_blendedLutShaperParams;
            AZStd::array<u32, 3> m_blendedLutDimensions;

            float m_weights[LookModificationSettings::MaxBlendLuts + 1]; // The first index is reserved for the weight of the non color graded value
            Render::ShaperParams m_colorGradingShaperParams[LookModificationSettings::MaxBlendLuts];
            Render::DisplayMapperAssetLut m_colorGradingLuts[LookModificationSettings::MaxBlendLuts];

            bool m_needToUpdateLut = false;
            uint32_t m_numSourceLuts = LookModificationSettings::MaxBlendLuts + 1; // Initalize to an invalid value
            HashValue64 m_lutBlendHash = HashValue64{ 0 };

            struct ShaderVariantInfo
            {
                const bool m_isFullyBaked = false;
                const RHI::PipelineState* m_pipelineState = nullptr;
            };

            AZStd::vector<ShaderVariantInfo> m_shaderVariant;
            RPI::ShaderVariantKey m_currentShaderVariantKeyFallbackValue;
            uint32_t m_currentShaderVariantIndex = 0;
        };
    }   // namespace Render
}   // namespace AZ
