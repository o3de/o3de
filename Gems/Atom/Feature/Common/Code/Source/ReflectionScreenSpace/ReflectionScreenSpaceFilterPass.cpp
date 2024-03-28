/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceFilterPass.h"
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
        RPI::Ptr<ReflectionScreenSpaceFilterPass> ReflectionScreenSpaceFilterPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceFilterPass> pass = aznew ReflectionScreenSpaceFilterPass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceFilterPass::ReflectionScreenSpaceFilterPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceFilterPass::CreateHistoryAttachmentImage(RPI::Ptr<RPI::PassAttachment>& historyAttachment)
        {
            Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            m_currentHistoryAttachmentImage = (m_currentHistoryAttachmentImage + 1) % ImageFrameCount;

            RHI::ImageViewDescriptor viewDesc = RHI::ImageViewDescriptor::Create(RHI::Format::R16G16B16A16_FLOAT, 0, 0);
            RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0, 0, 0, 0);
            m_historyAttachmentImage[m_currentHistoryAttachmentImage] = RPI::AttachmentImage::Create(*pool.get(), historyAttachment->m_descriptor.m_image, Name(historyAttachment->m_path.GetCStr()), &clearValue, &viewDesc);

            historyAttachment->m_importedResource = m_historyAttachmentImage[m_currentHistoryAttachmentImage];
        }

        void ReflectionScreenSpaceFilterPass::BuildInternal()
        {
            // retrieve the input reflection image descriptor to get the size
            RHI::ImageDescriptor reflectionImageDesc = m_ownedAttachments[0]->m_descriptor.m_image;
            
            // create the history attachment
            RHI::ImageBindFlags imageBindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;
            RHI::ImageDescriptor historyImageDesc = RHI::ImageDescriptor::Create2D(imageBindFlags, reflectionImageDesc.m_size.m_width, reflectionImageDesc.m_size.m_height, RHI::Format::R16G16B16A16_FLOAT);
            
            RPI::Ptr<RPI::PassAttachment> historyAttachment = aznew RPI::PassAttachment();
            AZStd::string historyAttachmentName = AZStd::string::format("%s.ReflectionScreenSpace_HistoryImage", GetPathName().GetCStr());
            historyAttachment->m_name = historyAttachmentName;
            historyAttachment->m_path = historyAttachmentName;
            historyAttachment->m_lifetime = RHI::AttachmentLifetimeType::Imported;
            historyAttachment->m_descriptor = historyImageDesc;
            m_ownedAttachments.push_back(historyAttachment);

            CreateHistoryAttachmentImage(historyAttachment);

            // bind the attachment
            m_historyAttachmentBinding = FindAttachmentBinding(AZ::Name("History"));
            m_historyAttachmentBinding->SetAttachment(historyAttachment);

            // retrieve reflection input attachment
            m_reflectionInputAttachmentBinding = FindAttachmentBinding(AZ::Name("ScreenSpaceReflectionInputOutput"));

            FullscreenTrianglePass::BuildInternal();
        }

        void ReflectionScreenSpaceFilterPass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            SpecularReflectionsFeatureProcessor* specularReflectionsFeatureProcessor = scene->GetFeatureProcessor<SpecularReflectionsFeatureProcessor>();
            AZ_Assert(specularReflectionsFeatureProcessor, "ReflectionScreenSpaceFilterPass requires the SpecularReflectionsFeatureProcessor");

            // retrieve reflection input attachment
            const RPI::Ptr<RPI::PassAttachment>& reflectionInputAttachment = m_reflectionInputAttachmentBinding->GetAttachment();
            AZ_Assert(reflectionInputAttachment, "ReflectionScreenSpaceFilterPass: Reflection input binding has no attachment!");
            RHI::ImageDescriptor& reflectionImageDescriptor = reflectionInputAttachment->m_descriptor.m_image;

            // retrieve output attachment
            RPI::PassAttachmentBinding& outputBinding = GetOutputBinding(0);
            const RPI::Ptr<RPI::PassAttachment>& outputAttachment = outputBinding.GetAttachment();
            AZ_Assert(outputAttachment, "ReflectionScreenSpaceFilterPass: Output binding has no attachment!");

            RHI::ImageDescriptor& outputImageDescriptor = outputAttachment->m_descriptor.m_image;

            const SSROptions& ssrOptions = specularReflectionsFeatureProcessor->GetSSROptions();

            m_shaderResourceGroup->SetConstant(m_invOutputScaleNameIndex, 1.0f / ssrOptions.GetOutputScale());
            m_shaderResourceGroup->SetConstant(m_outputWidthNameIndex, outputImageDescriptor.m_size.m_width);
            m_shaderResourceGroup->SetConstant(m_outputHeightNameIndex, outputImageDescriptor.m_size.m_height);
            m_shaderResourceGroup->SetConstant(m_mipLevelsNameIndex, aznumeric_cast<uint32_t>(reflectionImageDescriptor.m_mipLevels));
            m_shaderResourceGroup->SetConstant(m_coneTracingNameIndex, ssrOptions.m_coneTracing);
            m_shaderResourceGroup->SetConstant(m_rayTracingNameIndex, ssrOptions.IsRayTracingEnabled());
            m_shaderResourceGroup->SetConstant(m_temporalFilteringNameIndex, ssrOptions.m_temporalFiltering);
            m_shaderResourceGroup->SetConstant(m_invTemporalFilteringStrengthNameIndex, 1.0f / ssrOptions.m_temporalFilteringStrength);
            m_shaderResourceGroup->SetConstant(m_maxRoughnessNameIndex, ssrOptions.m_maxRoughness);
            m_shaderResourceGroup->SetConstant(m_roughnessBiasNameIndex, ssrOptions.m_roughnessBias);
            m_shaderResourceGroup->SetConstant(m_luminanceClampNameIndex, ssrOptions.m_luminanceClamp);
            m_shaderResourceGroup->SetConstant(m_maxLuminanceNameIndex, ssrOptions.m_maxLuminance);

            FullscreenTrianglePass::CompileResources(context);
        }

        void ReflectionScreenSpaceFilterPass::FrameEndInternal()
        {
            RPI::Ptr<RPI::PassAttachment> historyAttachment = m_historyAttachmentBinding->GetAttachment();
            historyAttachment->Update();

            RHI::Size& historyImageSize = historyAttachment->m_descriptor.m_image.m_size;
            RHI::Size& reflectionImageSize = m_ownedAttachments[0]->m_descriptor.m_image.m_size;

            if (historyImageSize != reflectionImageSize)
            {              
                historyImageSize = reflectionImageSize;

                CreateHistoryAttachmentImage(historyAttachment);
                m_historyAttachmentBinding->SetAttachment(historyAttachment);
            }

            FullscreenTrianglePass::FrameEndInternal();
        }

    }   // namespace RPI
}   // namespace AZ
