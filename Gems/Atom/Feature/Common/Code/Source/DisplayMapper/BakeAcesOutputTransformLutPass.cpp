/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DisplayMapper/BakeAcesOutputTransformLutPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<BakeAcesOutputTransformLutPass> BakeAcesOutputTransformLutPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<BakeAcesOutputTransformLutPass> pass = aznew BakeAcesOutputTransformLutPass(descriptor);
            return pass;
        }

        BakeAcesOutputTransformLutPass::BakeAcesOutputTransformLutPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
            m_needToUpdateLut = true;
        }

        BakeAcesOutputTransformLutPass::~BakeAcesOutputTransformLutPass()
        {
            ReleaseLutImage();
        }

        void BakeAcesOutputTransformLutPass::InitializeInternal()
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "BakeAcesOutputTransformLutPass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderInputColorMatIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_XYZtoDisplayPrimaries" });
                m_shaderInputCinemaLimitsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_cinemaLimits" });
                m_shaderInputAcesSplineParamsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_acesSplineParams" });
                m_shaderInputFlagsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_outputDisplayTransformFlags" });
                m_shaderInputOutputModeIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_outputDisplayTransformMode" });
                m_shaderInputSurroundGammaIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_surroundGamma" });
                m_shaderInputGammaIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_gamma" });

                m_shaderInputLutImageIndex = m_shaderResourceGroup->FindShaderInputImageIndex(Name{ "m_lutTexture" });

                m_shaderInputShaperBiasIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_shaperBias" });
                m_shaderInputShaperScaleIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_shaperScale" });
            }
        }

        void BakeAcesOutputTransformLutPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            ComputePass::SetupFrameGraphDependencies(frameGraph);

            if (m_displayMapperLut.m_lutImage == nullptr)
            {
                AcquireLutImage();
            }

            AZ_Assert(m_displayMapperLut.m_lutImage != nullptr, "BakeAcesOutputTransformLutPass unable to acquire LUT image");

            AZ::RHI::AttachmentId imageAttachmentId = AZ::RHI::AttachmentId("DisplayMapperLutImageAttachmentId");
            [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(imageAttachmentId, m_displayMapperLut.m_lutImage.get());
            AZ_Error("BakeAcesOutputTransformLutPass", result == RHI::ResultCode::Success, "Failed to import compute buffer with error %d", result);

            RHI::ImageScopeAttachmentDescriptor desc;
            desc.m_attachmentId = imageAttachmentId;
            desc.m_imageViewDescriptor = m_displayMapperLut.m_lutImageViewDescriptor;
            desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

            frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
        }

        void BakeAcesOutputTransformLutPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "BakeAcesOutputTransformLutPass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderResourceGroup->SetConstant(m_shaderInputColorMatIndex, m_displayMapperParameters.m_XYZtoDisplayPrimaries);
                m_shaderResourceGroup->SetConstant(m_shaderInputCinemaLimitsIndex, m_displayMapperParameters.m_cinemaLimits);
                m_shaderResourceGroup->SetConstantRaw(m_shaderInputAcesSplineParamsIndex, &m_displayMapperParameters.m_acesSplineParams, sizeof(SegmentedSplineParamsC9));
                m_shaderResourceGroup->SetConstant(m_shaderInputFlagsIndex, m_displayMapperParameters.m_OutputDisplayTransformFlags);
                m_shaderResourceGroup->SetConstant(m_shaderInputOutputModeIndex, m_displayMapperParameters.m_OutputDisplayTransformMode);
                m_shaderResourceGroup->SetConstant(m_shaderInputSurroundGammaIndex, m_displayMapperParameters.m_surroundGamma);
                m_shaderResourceGroup->SetConstant(m_shaderInputGammaIndex, m_displayMapperParameters.m_gamma);

                m_shaderResourceGroup->SetImageView(m_shaderInputLutImageIndex, m_displayMapperLut.m_lutImageView.get());

                m_shaderResourceGroup->SetConstant(m_shaderInputShaperBiasIndex, m_shaperParams.m_bias);
                m_shaderResourceGroup->SetConstant(m_shaderInputShaperScaleIndex, m_shaperParams.m_scale);
            }

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void BakeAcesOutputTransformLutPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            if (m_needToUpdateLut)
            {
                ComputePass::BuildCommandListInternal(context);

                m_needToUpdateLut = false;
            }
        }

        void BakeAcesOutputTransformLutPass::AcquireLutImage()
        {
            auto displayMapper = m_pipeline->GetScene()->GetFeatureProcessor<AZ::Render::AcesDisplayMapperFeatureProcessor>();
            displayMapper->GetDisplayMapperLut(m_displayMapperLut);
        }

        void BakeAcesOutputTransformLutPass::ReleaseLutImage()
        {
            m_displayMapperLut.m_lutImage.reset();
            m_displayMapperLut.m_lutImageView.reset();
            m_displayMapperLut = {};
        }

        void BakeAcesOutputTransformLutPass::SetDisplayBufferFormat(RHI::Format format)
        {
            if (m_displayBufferFormat != format)
            {
                m_needToUpdateLut = true;
                m_displayBufferFormat = format;
                m_outputDeviceTransformType = AcesDisplayMapperFeatureProcessor::GetOutputDeviceTransformType(m_displayBufferFormat);
                AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, m_outputDeviceTransformType);
                m_shaperParams = GetAcesShaperParameters(m_outputDeviceTransformType);
            }
        }

        const ShaperParams& BakeAcesOutputTransformLutPass::GetShaperParams() const
        {
            return m_shaperParams;
        }
    }   // namespace Render
}   // namespace AZ
