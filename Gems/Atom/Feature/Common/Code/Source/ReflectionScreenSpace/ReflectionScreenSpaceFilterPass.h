/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>

namespace AZ
{
    namespace Render
    {
        //! This pass filters the screenspace reflection image
        class ReflectionScreenSpaceFilterPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceFilterPass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceFilterPass, "{54F8F4FC-73DD-4312-B474-3CCB3AAE216A}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceFilterPass, SystemAllocator, 0);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceFilterPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionScreenSpaceFilterPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildInternal() override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void FrameEndInternal() override;

            void CreateHistoryAttachmentImage(RPI::Ptr<RPI::PassAttachment>& historyAttachment);

            static const uint32_t ImageFrameCount = 3;
            Data::Instance<RPI::AttachmentImage> m_historyAttachmentImage[ImageFrameCount];
            uint32_t m_currentHistoryAttachmentImage = 0;

            RPI::PassAttachmentBinding* m_historyAttachmentBinding = nullptr;
            RPI::PassAttachmentBinding* m_reflectionInputAttachmentBinding = nullptr;

            RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            RHI::ShaderInputNameIndex m_outputWidthNameIndex = "m_outputWidth";
            RHI::ShaderInputNameIndex m_outputHeightNameIndex = "m_outputHeight";
            RHI::ShaderInputNameIndex m_mipLevelsNameIndex = "m_mipLevels";
            RHI::ShaderInputNameIndex m_coneTracingNameIndex = "m_coneTracing";
            RHI::ShaderInputNameIndex m_rayTracingNameIndex = "m_rayTracing";
            RHI::ShaderInputNameIndex m_temporalFilteringNameIndex = "m_temporalFiltering";
            RHI::ShaderInputNameIndex m_invTemporalFilteringStrengthNameIndex = "m_invTemporalFilteringStrength";
            RHI::ShaderInputNameIndex m_maxRoughnessNameIndex = "m_maxRoughness";
            RHI::ShaderInputNameIndex m_roughnessBiasNameIndex = "m_roughnessBias";
            RHI::ShaderInputNameIndex m_luminanceClampNameIndex = "m_luminanceClamp";
            RHI::ShaderInputNameIndex m_maxLuminanceNameIndex = "m_maxLuminance";
        };
    }   // namespace RPI
}   // namespace AZ
