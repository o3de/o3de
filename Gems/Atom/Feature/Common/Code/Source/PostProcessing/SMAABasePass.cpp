/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

#include <PostProcessing/SMAABasePass.h>

namespace AZ
{
    namespace Render
    {
        SMAABasePass::SMAABasePass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        SMAABasePass::~SMAABasePass()
        {
        }

        void SMAABasePass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            AZ_Assert(m_shaderResourceGroup != nullptr, "SMAABasePass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            UpdateCurrentShaderVariant();
        }

        void SMAABasePass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "SMAABasePass %s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            BindPassSrg(context, m_shaderResourceGroup);

            AZ::Vector4 renderTargetMetrics = CalculateRenderTargetMetrics(GetOutputBinding(0).GetAttachment().get());
            if (renderTargetMetrics.GetX() != m_renderTargetMetrics.GetX() ||
                renderTargetMetrics.GetY() != m_renderTargetMetrics.GetY())
            {
                m_renderTargetMetrics = renderTargetMetrics;
                InvalidateSRG();
            }

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            if (m_needToUpdateSRG)
            {
                UpdateSRG();
                m_needToUpdateSRG = false;
            }

            m_shaderResourceGroup->Compile();
        }

        void SMAABasePass::UpdateCurrentShaderVariant()
        {
            auto shaderOption = m_shader->CreateShaderOptionGroup();

            GetCurrentShaderOption(shaderOption);
            shaderOption.SetUnspecifiedToDefaultValues();
            UpdateShaderOptions(shaderOption.GetShaderVariantId());
            m_needToUpdateShaderVariant = false;
            InvalidateSRG();
        }

        void SMAABasePass::InvalidateShaderVariant()
        {
            m_needToUpdateShaderVariant = true;
        }

        void SMAABasePass::InvalidateSRG()
        {
            m_needToUpdateSRG = true;
        }

        AZ::Vector4 SMAABasePass::CalculateRenderTargetMetrics(const RPI::PassAttachment* attachment)
        {
            AZ_Assert(attachment != nullptr, "Null PassAttachment pointer have been passed in SMAABasePass::CalculateRenderTargetMetrics().");

            const RPI::PassAttachmentBinding* sizeSource = attachment->m_sizeSource;
            AZ_Assert(sizeSource != nullptr, "Binding sizeSource of attachment is null.");
            AZ_Assert(sizeSource->GetAttachment() != nullptr, "Attachment of sizeSource is null.");
            AZ::RHI::Size size = sizeSource->GetAttachment()->m_descriptor.m_image.m_size;

            return AZ::Vector4(
                1.0f / static_cast<float>(size.m_width),
                1.0f / static_cast<float>(size.m_height),
                static_cast<float>(size.m_width),
                static_cast<float>(size.m_height));
        }

    }   // namespace Render
}   // namespace AZ
