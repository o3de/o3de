/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionScreenSpaceCompositePass.h"
#include "ReflectionScreenSpaceBlurPass.h"
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/SpecularReflections/SpecularReflectionsFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ReflectionScreenSpaceCompositePass> ReflectionScreenSpaceCompositePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ReflectionScreenSpaceCompositePass> pass = aznew ReflectionScreenSpaceCompositePass(descriptor);
            return AZStd::move(pass);
        }

        ReflectionScreenSpaceCompositePass::ReflectionScreenSpaceCompositePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void ReflectionScreenSpaceCompositePass::CompileResources([[maybe_unused]] const RHI::FrameGraphCompileContext& context)
        {
            if (!m_shaderResourceGroup)
            {
                return;
            }

            RPI::Scene* scene = m_pipeline->GetScene();
            SpecularReflectionsFeatureProcessorInterface* specularReflectionsFeatureProcessor = scene->GetFeatureProcessor<SpecularReflectionsFeatureProcessorInterface>();
            AZ_Assert(specularReflectionsFeatureProcessor, "ReflectionScreenSpaceCompositePass requires the SpecularReflectionsFeatureProcessor");

            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();
            AZ_Assert(outputAttachment, "ReflectionScreenSpaceCompositePass: Output binding has no attachment!");
            RHI::Size outputImageSize = outputAttachment->m_descriptor.m_image.m_size;

            const SSROptions& ssrOptions = specularReflectionsFeatureProcessor->GetSSROptions();

            m_shaderResourceGroup->SetConstant(m_outputScaleNameIndex, ssrOptions.GetOutputScale());
            m_shaderResourceGroup->SetConstant(m_outputWidthNameIndex, outputImageSize.m_width);
            m_shaderResourceGroup->SetConstant(m_outputHeightNameIndex, outputImageSize.m_height);
            m_shaderResourceGroup->SetConstant(m_maxRoughnessNameIndex, ssrOptions.m_maxRoughness);

            FullscreenTrianglePass::CompileResources(context);
        }
    }   // namespace RPI
}   // namespace AZ
