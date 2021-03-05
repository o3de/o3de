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

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Shader/ShaderVariant.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/PipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <PostProcessing/SMAACommon.h>
#include <PostProcessing/SMAANeighborhoodBlendingPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<SMAANeighborhoodBlendingPass> SMAANeighborhoodBlendingPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew SMAANeighborhoodBlendingPass(descriptor);
        }

        SMAANeighborhoodBlendingPass::SMAANeighborhoodBlendingPass(const RPI::PassDescriptor& descriptor)
            : SMAABasePass(descriptor)
            , m_blendingOutputModeOptionName(BlendingOutputModeOptionName)
        {
        }

        void SMAANeighborhoodBlendingPass::Init()
        {
            SMAABasePass::Init();

            AZ_Assert(m_shaderResourceGroup != nullptr, "SMAANeighborhoodBlendingPass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            m_renderTargetMetricsShaderInputIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_renderTargetMetrics" });
        }

        void SMAANeighborhoodBlendingPass::UpdateSRG()
        {
            m_shaderResourceGroup->SetConstant(m_renderTargetMetricsShaderInputIndex, m_renderTargetMetrics);
        }

        void SMAANeighborhoodBlendingPass::GetCurrentShaderOption(AZ::RPI::ShaderOptionGroup& shaderOption) const
        {
            switch (m_outputMode)
            {
            case SMAAOutputMode::BlendResult:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::BlendResult"));
                break;
            case SMAAOutputMode::PassThrough:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::PassThrough"));
                break;
            case SMAAOutputMode::EdgeTexture:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::EdgeTexture"));
                break;
            case SMAAOutputMode::BlendWeightTexture:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::BlendWeightTexture"));
                break;
            case SMAAOutputMode::BlendResultWithProvisionalTonemap:
            default:
                shaderOption.SetValue(m_blendingOutputModeOptionName, AZ::Name("BlendingOutputMode::BlendResultWithProvisionalTonemap"));
                break;
            }
        }

        void SMAANeighborhoodBlendingPass::SetOutputMode(SMAAOutputMode mode)
        {
            if (m_outputMode != mode)
            {
                m_outputMode = mode;
                InvalidateShaderVariant();
            }
        }
    }   // namespace Render
}   // namespace AZ
