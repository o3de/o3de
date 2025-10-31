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
        //! This pass traces screenspace reflections from the previous frame image.
        class ReflectionScreenSpaceTracePass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceTracePass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceTracePass, "{70FD45E9-8363-4AA1-A514-3C24AC975E53}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceTracePass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceTracePass> Create(const RPI::PassDescriptor& descriptor);

            Data::Instance<RPI::AttachmentImage>& GetPreviousFrameImageAttachment() { return m_previousFrameImageAttachment; }

        private:
            explicit ReflectionScreenSpaceTracePass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            void BuildInternal() override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            Data::Instance<RPI::AttachmentImage> m_previousFrameImageAttachment;

            RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            RHI::ShaderInputNameIndex m_outputWidthNameIndex = "m_outputWidth";
            RHI::ShaderInputNameIndex m_outputHeightNameIndex = "m_outputHeight";
            RHI::ShaderInputNameIndex m_rayTracingEnabledNameIndex = "m_rayTracingEnabled";
            RHI::ShaderInputNameIndex m_rayTraceFallbackDataNameIndex = "m_rayTraceFallbackData";
            RHI::ShaderInputNameIndex m_maxRayDistanceNameIndex = "m_maxRayDistance";
            RHI::ShaderInputNameIndex m_maxDepthThresholdNameIndex = "m_maxDepthThreshold";
            RHI::ShaderInputNameIndex m_maxRoughnessNameIndex = "m_maxRoughness";
            RHI::ShaderInputNameIndex m_roughnessBiasNameIndex = "m_roughnessBias";
        };
    }   // namespace RPI
}   // namespace AZ
