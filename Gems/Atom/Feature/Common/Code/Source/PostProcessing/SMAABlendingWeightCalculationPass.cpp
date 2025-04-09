/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <PostProcessing/SMAACommon.h>
#include <PostProcessing/SMAABlendingWeightCalculationPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<SMAABlendingWeightCalculationPass> SMAABlendingWeightCalculationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew SMAABlendingWeightCalculationPass(descriptor);
        }

        SMAABlendingWeightCalculationPass::SMAABlendingWeightCalculationPass(const RPI::PassDescriptor& descriptor)
            : SMAABasePass(descriptor)
            , m_enableDiagonalDetectionFeatureOptionName(EnableDiagonalDetectionFeatureOptionName)
            , m_enableCornerDetectionFeatureOptionName(EnableCornerDetectionFeatureOptionName)
        {
        }

        void SMAABlendingWeightCalculationPass::SetMaxSearchSteps(int steps)
        {
            if (m_maxSearchSteps != steps)
            {
                m_maxSearchSteps = steps;
                InvalidateSRG();
            }
        }

        void SMAABlendingWeightCalculationPass::SetMaxSearchStepsDiagonal(int steps)
        {
            if (m_maxSearchStepsDiagonal != steps)
            {
                m_maxSearchStepsDiagonal = steps;
                InvalidateSRG();
            }
        }

        void SMAABlendingWeightCalculationPass::SetCornerRounding(int cornerRounding)
        {
            if (m_cornerRounding != cornerRounding)
            {
                m_cornerRounding = cornerRounding;
                InvalidateSRG();
            }
        }

        void SMAABlendingWeightCalculationPass::SetDiagonalDetectionEnable(bool enable)
        {
            if (m_enableDiagonalDetection != enable)
            {
                m_enableDiagonalDetection = enable;
                InvalidateShaderVariant();
            }
        }

        void SMAABlendingWeightCalculationPass::SetCornerDetectionEnable(bool enable)
        {
            if (m_enableCornerDetection != enable)
            {
                m_enableCornerDetection = enable;
                InvalidateShaderVariant();
            }
        }

        void SMAABlendingWeightCalculationPass::InitializeInternal()
        {
            SMAABasePass::InitializeInternal();

            AZ_Assert(m_shaderResourceGroup != nullptr, "SMAABlendingWeightCalculationPass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            m_areaTexture = AZ::RPI::LoadStreamingTexture(PathToSMAAAreaTexture);
            m_searchTexture = AZ::RPI::LoadStreamingTexture(PathToSMAASearchTexture);

            m_areaTextureShaderInputIndex.Reset();
            m_searchTextureShaderInputIndex.Reset();
            m_renderTargetMetricsShaderInputIndex.Reset();
            m_maxSearchStepsShaderInputIndex.Reset();
            m_maxSearchStepsDiagonalShaderInputIndex.Reset();
            m_cornerRoundingShaderInputIndex.Reset();
        }

        void SMAABlendingWeightCalculationPass::UpdateSRG()
        {
            m_shaderResourceGroup->SetConstant(m_renderTargetMetricsShaderInputIndex, m_renderTargetMetrics);
            m_shaderResourceGroup->SetImage(m_areaTextureShaderInputIndex, m_areaTexture);
            m_shaderResourceGroup->SetImage(m_searchTextureShaderInputIndex, m_searchTexture);
            m_shaderResourceGroup->SetConstant(m_maxSearchStepsShaderInputIndex, m_maxSearchSteps);
            m_shaderResourceGroup->SetConstant(m_maxSearchStepsDiagonalShaderInputIndex, m_maxSearchStepsDiagonal);
            m_shaderResourceGroup->SetConstant(m_cornerRoundingShaderInputIndex, m_cornerRounding);
        }

        void SMAABlendingWeightCalculationPass::GetCurrentShaderOption(AZ::RPI::ShaderOptionGroup& shaderOption) const
        {
            shaderOption.SetValue(m_enableDiagonalDetectionFeatureOptionName, m_enableDiagonalDetection ? AZ::Name("true") : AZ::Name("false"));
            shaderOption.SetValue(m_enableCornerDetectionFeatureOptionName, m_enableCornerDetection ? AZ::Name("true") : AZ::Name("false"));
        }
    }   // namespace Render
}   // namespace AZ
