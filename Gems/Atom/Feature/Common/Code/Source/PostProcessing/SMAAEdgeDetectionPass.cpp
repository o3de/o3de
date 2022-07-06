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

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <PostProcessing/SMAACommon.h>
#include <PostProcessing/SMAAEdgeDetectionPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<SMAAEdgeDetectionPass> SMAAEdgeDetectionPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew SMAAEdgeDetectionPass(descriptor);
        }

        SMAAEdgeDetectionPass::SMAAEdgeDetectionPass(const RPI::PassDescriptor& descriptor)
            : SMAABasePass(descriptor)
            , m_enablePredicationFeatureOptionName(EnablePredicationFeatureOptionName)
            , m_edgeDetectionModeOptionName(EdgeDetectionModeOptionName)
        {
        }

        void SMAAEdgeDetectionPass::InitializeInternal()
        {
            SMAABasePass::InitializeInternal();

            m_renderTargetMetricsShaderInputIndex.Reset();
            m_chromaThresholdShaderInputIndex.Reset();
            m_depthThresholdShaderInputIndex.Reset();
            m_localContrastAdaptationFactorShaderInputIndex.Reset();
            m_predicationThresholdShaderInputIndex.Reset();
            m_predicationScaleShaderInputIndex.Reset();
            m_predicationStrengthShaderInputIndex.Reset();
        }


        void SMAAEdgeDetectionPass::UpdateSRG()
        {
            m_shaderResourceGroup->SetConstant(m_renderTargetMetricsShaderInputIndex, m_renderTargetMetrics);
            m_shaderResourceGroup->SetConstant(m_chromaThresholdShaderInputIndex, m_chromaThreshold);
            m_shaderResourceGroup->SetConstant(m_depthThresholdShaderInputIndex, m_depthThreshold);
            m_shaderResourceGroup->SetConstant(m_localContrastAdaptationFactorShaderInputIndex, m_localContrastAdaptationFactor);
            m_shaderResourceGroup->SetConstant(m_predicationThresholdShaderInputIndex, m_predicationThreshold);
            m_shaderResourceGroup->SetConstant(m_predicationScaleShaderInputIndex, m_predicationScale);
            m_shaderResourceGroup->SetConstant(m_predicationStrengthShaderInputIndex, m_predicationStrength);
        }

        void SMAAEdgeDetectionPass::GetCurrentShaderOption(AZ::RPI::ShaderOptionGroup& shaderOption) const
        {
            shaderOption.SetValue(m_enablePredicationFeatureOptionName, m_predicationEnable ? AZ::Name("true") : AZ::Name("false"));
            switch (m_edgeDetectionMode)
            {
                case SMAAEdgeDetectionMode::Depth:
                    shaderOption.SetValue(m_edgeDetectionModeOptionName, AZ::Name("EdgeDetectionMode::Depth"));
                    break;
                case SMAAEdgeDetectionMode::Luma:
                    shaderOption.SetValue(m_edgeDetectionModeOptionName, AZ::Name("EdgeDetectionMode::Luma"));
                    break;
                case SMAAEdgeDetectionMode::Color:
                default:
                    shaderOption.SetValue(m_edgeDetectionModeOptionName, AZ::Name("EdgeDetectionMode::Color"));
                    break;
            }
        }

        void SMAAEdgeDetectionPass::SetEdgeDetectionMode(SMAAEdgeDetectionMode mode)
        {
            if (m_edgeDetectionMode != mode)
            {
                m_edgeDetectionMode = mode;
                InvalidateShaderVariant();
            }
        }

        void SMAAEdgeDetectionPass::SetChromaThreshold(float threshold)
        {
            if (m_chromaThreshold != threshold)
            {
                m_chromaThreshold = threshold;
                InvalidateSRG();
            }
        }

        void SMAAEdgeDetectionPass::SetDepthThreshold(float threshold)
        {
            if (m_depthThreshold != threshold)
            {
                m_depthThreshold = threshold;
                InvalidateSRG();
            }
        }

        void SMAAEdgeDetectionPass::SetLocalContrastAdaptationFactor(float factor)
        {
            if (m_localContrastAdaptationFactor != factor)
            {
                m_localContrastAdaptationFactor = factor;
                InvalidateSRG();
            }
        }

        void SMAAEdgeDetectionPass::SetPredicationEnable(bool enable)
        {
            if (m_predicationEnable != enable)
            {
                m_predicationEnable = enable;
                InvalidateShaderVariant();
            }
        }

        void SMAAEdgeDetectionPass::SetPredicationThreshold(float threshold)
        {
            if (m_predicationThreshold != threshold)
            {
                m_predicationThreshold = threshold;
                InvalidateSRG();
            }
        }

        void SMAAEdgeDetectionPass::SetPredicationScale(float scale)
        {
            if (m_predicationScale != scale)
            {
                m_predicationScale = scale;
                InvalidateSRG();
            }
        }

        void SMAAEdgeDetectionPass::SetPredicationStrength(float strength)
        {
            if (m_predicationStrength != strength)
            {
                m_predicationStrength = strength;
                InvalidateSRG();
            }
        }

    }   // namespace Render
}   // namespace AZ
