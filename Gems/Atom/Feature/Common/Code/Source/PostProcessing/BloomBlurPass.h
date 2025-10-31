/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>

#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/RPI.Reflect/Pass/DownsampleMipChainPassData.h>

#include <Atom/RHI/ConstantsData.h>
#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>

namespace AZ
{
    namespace Render
    {
        class BloomBlurChildPass;

        // Pass for blurring downsampled images
        class BloomBlurPass
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(BloomBlurPass);

        public:
            AZ_RTTI(BloomBlurPass, "{02E41C48-5CC7-4277-B66E-009E4D6D32BA}", ParentPass);
            AZ_CLASS_ALLOCATOR(BloomBlurPass, SystemAllocator);
            virtual ~BloomBlurPass() = default;

            //! Creates a BloomBlurPass
            static RPI::Ptr<BloomBlurPass> Create(const RPI::PassDescriptor& descriptor);
        protected:
            BloomBlurPass(const RPI::PassDescriptor& descriptor);

            // Pass behaviour overrides...
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void GetInputInfo();
            void UpdateParameters();
            void BuildKernelData();
            void BuildChildPasses();
            void UpdateChildren();
            void CreateBinding(BloomBlurChildPass* pass, uint32_t mipLevel, bool isHorizontalPass);

            void GenerateWeightOffset(float sigma, uint32_t kernelRadius);
            void PrepareBuffer(uint32_t blurStageIndex);

            float KernelRadiusClamp(float radius);

            float Gaussian1D(float x, float sigma);

            RPI::DownsampleMipChainPassData m_passData;

            AZStd::vector<AZStd::vector<float>> m_weightData;
            AZStd::vector<AZStd::vector<float>> m_offsetData;
            AZStd::vector<uint32_t> m_kernelRadiusData;

            AZStd::vector<Data::Instance<RPI::Buffer>> m_weightBuffer;
            AZStd::vector<Data::Instance<RPI::Buffer>> m_offsetBuffer;

            AZStd::vector<float> m_kernelScreenPercents = {
                AZ::Render::Bloom::DefaultScreenPercentStage0,
                AZ::Render::Bloom::DefaultScreenPercentStage1,
                AZ::Render::Bloom::DefaultScreenPercentStage2,
                AZ::Render::Bloom::DefaultScreenPercentStage3,
                AZ::Render::Bloom::DefaultScreenPercentStage4
            };

            float m_kernelSizeScale = AZ::Render::Bloom::DefaultKernelSizeScale;

            uint32_t m_inputWidth = 0;
            uint32_t m_inputHeight = 0;

            uint8_t m_stageCount = AZ::Render::Bloom::MaxStageCount;

            bool m_paramsUpdated = true;
        };

        // Child pass spawned by parent blur pass, one child do gaussian blur on single downsampled level in one direction
        class BloomBlurChildPass
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(BloomBlurChildPass);

            friend class BloomBlurPass;

        public:
            AZ_RTTI(BloomBlurChildPass, "{85D3FE9B-D347-40D6-B666-B4DF855F5B80}", RenderPass);
            AZ_CLASS_ALLOCATOR(BloomBlurChildPass, SystemAllocator);
            virtual ~BloomBlurChildPass() = default;

            //! Creates a BloomBlurChildPass
            static RPI::Ptr<BloomBlurChildPass> Create(const RPI::PassDescriptor& descriptor);

            void UpdateParameters(
                Data::Instance<RPI::Buffer> offsetBuffer,
                Data::Instance<RPI::Buffer> weightBuffer,
                uint32_t radius,
                bool direction,
                uint32_t mipLevel,
                uint32_t imageWidth,
                uint32_t imageHeight);

        protected:
            BloomBlurChildPass(const RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            // output texture vertical dimension required by compute shader
            RHI::ShaderInputNameIndex m_offsetsInputIndex = "m_offsets";
            RHI::ShaderInputNameIndex m_weightsInputIndex = "m_weights";

            RHI::ShaderInputNameIndex m_kernelRadiusInputIndex = "m_kernelRadius";
            RHI::ShaderInputNameIndex m_directionInputIndex = "m_direction";
            RHI::ShaderInputNameIndex m_sourceImageSizeInputIndex = "m_sourceImageSize";
            RHI::ShaderInputNameIndex m_sourceImageTexelSizeInputIndex = "m_sourceImageTexelSize";
            RHI::ShaderInputNameIndex m_mipLevelInputIndex = "m_mipLevel";

            Data::Instance<RPI::Buffer> m_offsetBuffer;
            Data::Instance<RPI::Buffer> m_weightBuffer;

            uint32_t m_sourceImageWidth;
            uint32_t m_sourceImageHeight;
        };
    }   // namespace RPI
}   // namespace AZ
