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

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RHI/ImagePool.h>

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
            AZ_CLASS_ALLOCATOR(BlendColorGradingLutsPass, SystemAllocator, 0);
            virtual ~BlendColorGradingLutsPass();

            /// Creates a DisplayMapperPass
            static RPI::Ptr<BlendColorGradingLutsPass> Create(const RPI::PassDescriptor& descriptor);

            void SetShaperParameters(const ShaperParams& shaperParams);

        private:
            explicit BlendColorGradingLutsPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            void Init();

            void AcquireLutImage();
            void ReleaseLutImage();

            void CheckLutBlendSettings();

            bool m_resourcesInitialized = false;

            const AZ::Name m_numSourceLutsShaderVariantOptionName;
            bool m_needToUpdateShaderVariant = false;

            RHI::ShaderInputImageIndex m_shaderInputBlendedLutImageIndex;
            RHI::ShaderInputConstantIndex m_shaderInputBlendedLutDimensionsIndex;
            RHI::ShaderInputConstantIndex m_shaderInputBlendedLutShaperTypeIndex;
            RHI::ShaderInputConstantIndex m_shaderInputBlendededLutShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderInputBlendededLutShaperScaleIndex;

            RHI::ShaderInputImageIndex m_shaderInputSourceLut1ImageIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut1ShaperTypeIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut1ShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut1ShaperScaleIndex;

            RHI::ShaderInputImageIndex m_shaderInputSourceLut2ImageIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut2ShaperTypeIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut2ShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut2ShaperScaleIndex;

            RHI::ShaderInputImageIndex m_shaderInputSourceLut3ImageIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut3ShaperTypeIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut3ShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut3ShaperScaleIndex;

            RHI::ShaderInputImageIndex m_shaderInputSourceLut4ImageIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut4ShaperTypeIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut4ShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSourceLut4ShaperScaleIndex;

            RHI::ShaderInputConstantIndex m_shaderInputWeight0Index;
            RHI::ShaderInputConstantIndex m_shaderInputWeight1Index;
            RHI::ShaderInputConstantIndex m_shaderInputWeight2Index;
            RHI::ShaderInputConstantIndex m_shaderInputWeight3Index;
            RHI::ShaderInputConstantIndex m_shaderInputWeight4Index;

            AZ::Render::DisplayMapperParameters m_displayMapperParameters = {};

            DisplayMapperLut m_blendedLut = {};
            ShaperParams m_blendedLutShaperParams;
            AZStd::array<u32, 3> m_blendedLutDimensions;

            float m_weights[LookModificationSettings::MaxBlendLuts + 1]; // The first index is reserved for the weight of the non color graded value
            Data::Asset<RPI::AnyAsset> m_colorGradingLutAssets[LookModificationSettings::MaxBlendLuts];
            ShaperPresetType m_colorGradingShaperPresets[LookModificationSettings::MaxBlendLuts];
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
