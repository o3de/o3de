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

            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceBlurPass"), GetRenderPipeline());

            RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                {
                    Render::ReflectionScreenSpaceBlurPass* blurPass = azrtti_cast<ReflectionScreenSpaceBlurPass*>(pass);

                    // compute the max mip level based on the available mips in the previous frame image, and capping it
                    // to stay within a range that has reasonable data
                    const uint32_t MaxNumRoughnessMips = 8;
                    uint32_t maxMipLevel = AZStd::min(MaxNumRoughnessMips, blurPass->GetNumBlurMips()) - 1;

                    auto constantIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_maxMipLevel"));
                    m_shaderResourceGroup->SetConstant(constantIndex, maxMipLevel);

                     return RPI::PassFilterExecutionFlow::StopVisitingPasses;
                });

            FullscreenTrianglePass::CompileResources(context);
        }
    }   // namespace RPI
}   // namespace AZ
