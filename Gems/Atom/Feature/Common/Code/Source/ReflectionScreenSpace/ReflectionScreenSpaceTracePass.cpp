/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceTracePass.h"
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <SpecularReflections/SpecularReflectionsFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionScreenSpaceTracePass> ReflectionScreenSpaceTracePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceTracePass> pass = aznew ReflectionScreenSpaceTracePass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceTracePass::ReflectionScreenSpaceTracePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceTracePass::BuildInternal()
        {
            Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            // retrieve the previous frame image attachment from the pass
            AZ_Assert(m_ownedAttachments.size() == 3, "ReflectionScreenSpaceTracePass must have the following attachment images defined: ReflectionImage, TraceCoordsImage, and PreviousFrameImage");
            RPI::Ptr<RPI::PassAttachment> previousFrameImageAttachment = m_ownedAttachments[2];
            
            // update the image attachment descriptor to sync up size and format
            previousFrameImageAttachment->Update();
            
            // change the lifetime since we want it to live between frames
            previousFrameImageAttachment->m_lifetime = RHI::AttachmentLifetimeType::Imported;
            
            // set the bind flags
            RHI::ImageDescriptor& imageDesc = previousFrameImageAttachment->m_descriptor.m_image;
            imageDesc.m_bindFlags |= RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;
            
            // create the image attachment
            RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0, 0, 0, 0);
            m_previousFrameImageAttachment = RPI::AttachmentImage::Create(*pool.get(), imageDesc, Name(previousFrameImageAttachment->m_path.GetCStr()), &clearValue, nullptr);

            previousFrameImageAttachment->m_path = m_previousFrameImageAttachment->GetAttachmentId();
            previousFrameImageAttachment->m_importedResource = m_previousFrameImageAttachment;
        }

        void ReflectionScreenSpaceTracePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            SpecularReflectionsFeatureProcessor* specularReflectionsFeatureProcessor = scene->GetFeatureProcessor<SpecularReflectionsFeatureProcessor>();
            AZ_Assert(specularReflectionsFeatureProcessor, "ReflectionScreenSpaceTracePass requires the SpecularReflectionsFeatureProcessor");

            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();
            AZ_Assert(outputAttachment, "ReflectionScreenSpaceTracePass: Output binding has no attachment!");
            RHI::Size outputImageSize = outputAttachment->m_descriptor.m_image.m_size;

            const SSROptions& ssrOptions = specularReflectionsFeatureProcessor->GetSSROptions();

            m_shaderResourceGroup->SetConstant(m_invOutputScaleNameIndex, 1.0f / ssrOptions.GetOutputScale());
            m_shaderResourceGroup->SetConstant(m_outputWidthNameIndex, outputImageSize.m_width);
            m_shaderResourceGroup->SetConstant(m_outputHeightNameIndex, outputImageSize.m_height);
            m_shaderResourceGroup->SetConstant(m_rayTracingEnabledNameIndex, ssrOptions.IsRayTracingEnabled());
            m_shaderResourceGroup->SetConstant(m_rayTraceFallbackDataNameIndex, ssrOptions.IsRayTracingFallbackEnabled());
            m_shaderResourceGroup->SetConstant(m_maxRayDistanceNameIndex, ssrOptions.m_maxRayDistance);
            m_shaderResourceGroup->SetConstant(m_maxDepthThresholdNameIndex, ssrOptions.m_maxDepthThreshold);
            m_shaderResourceGroup->SetConstant(m_maxRoughnessNameIndex, ssrOptions.m_maxRoughness);
            m_shaderResourceGroup->SetConstant(m_roughnessBiasNameIndex, ssrOptions.m_roughnessBias);

            FullscreenTrianglePass::CompileResources(context);
        }

    }   // namespace RPI
}   // namespace AZ
