/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/DisplayMapper/OutputTransformPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<OutputTransformPass> OutputTransformPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<OutputTransformPass> pass = aznew OutputTransformPass(descriptor);
            return AZStd::move(pass);
        }

        OutputTransformPass::OutputTransformPass(const RPI::PassDescriptor& descriptor)
            : DisplayMapperFullScreenPass(descriptor)
            , m_toneMapperShaderVariantOptionName(ToneMapperShaderVariantOptionName)
            , m_transferFunctionShaderVariantOptionName(TransferFunctionShaderVariantOptionName)
        {
        }

        OutputTransformPass::~OutputTransformPass()
        {
        }

        void OutputTransformPass::InitializeInternal()
        {
            DisplayMapperFullScreenPass::InitializeInternal();

            AZ_Assert(m_shaderResourceGroup != nullptr, "OutputTransformPass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderInputCinemaLimitsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_cinemaLimits" });
            }

            InitializeShaderVariant();
        }

        void OutputTransformPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            DeclareAttachmentsToFrameGraph(frameGraph);
            DeclarePassDependenciesToFrameGraph(frameGraph);

            frameGraph.SetEstimatedItemCount(1);
        }

        void OutputTransformPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "OutputTransformPass %s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            m_shaderResourceGroup->SetConstant(m_shaderInputCinemaLimitsIndex, m_displayMapperParameters.m_cinemaLimits);

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void OutputTransformPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "LookModificationPass %s has a null shader resource group when calling Execute.", GetPathName().GetCStr());

            RHI::CommandList* commandList = context.GetCommandList();

            commandList->SetViewport(m_viewportState);
            commandList->SetScissor(m_scissorState);

            SetSrgsForDraw(commandList);

            m_item.m_pipelineState = GetPipelineStateFromShaderVariant();

            commandList->Submit(m_item);
        }

        void OutputTransformPass::SetToneMapperType(const ToneMapperType& toneMapperType)
        {
            if (m_toneMapperType != toneMapperType)
            {
                m_toneMapperType = toneMapperType;
                m_needToUpdateShaderVariant = true;
            }
        }

        void OutputTransformPass::SetTransferFunctionType(const TransferFunctionType& transferFunctionType)
        {
            if (m_transferFunctionType != transferFunctionType)
            {
                m_transferFunctionType = transferFunctionType;
                m_needToUpdateShaderVariant = true;
            }
        }

        void OutputTransformPass::InitializeShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "OutputTransformPass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());

            AZStd::vector<AZ::Name> toneMapperVariationTypes = { AZ::Name("ToneMapperType::None"), AZ::Name("ToneMapperType::Reinhard")};
            AZStd::vector<AZ::Name> transferFunctionVariationTypes = {
                AZ::Name("TransferFunctionType::None"),
                AZ::Name("TransferFunctionType::Gamma22"),
                AZ::Name("TransferFunctionType::PerceptualQuantizer") };

            auto toneMapperVariationTypeCount = toneMapperVariationTypes.size();
            auto totalVariationCount = toneMapperVariationTypes.size() * transferFunctionVariationTypes.size();

            // Caching all pipeline state for each shader variation for performance reason.
            for (auto shaderVariantIndex = 0; shaderVariantIndex < totalVariationCount; ++shaderVariantIndex)
            {
                auto shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, toneMapperVariationTypes[shaderVariantIndex % toneMapperVariationTypeCount]);
                shaderOption.SetValue(m_transferFunctionShaderVariantOptionName, transferFunctionVariationTypes[shaderVariantIndex / toneMapperVariationTypeCount]);

                PreloadShaderVariant(m_shader, shaderOption, GetRenderAttachmentConfiguration(), GetMultisampleState());
            }

            m_needToUpdateShaderVariant = true;
        }

        void OutputTransformPass::UpdateCurrentShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "OutputTransformPass %s has a null shader when calling UpdateCurrentShaderVariant.", GetPathName().GetCStr());

            auto shaderOption = m_shader->CreateShaderOptionGroup();
            // Decide which shader to use.
            switch (m_toneMapperType)
            {
            case ToneMapperType::Reinhard:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::Reinhard"));
                break;
            default:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::None"));
                break;
            }
            switch (m_transferFunctionType)
            {
            case TransferFunctionType::Gamma22:
                shaderOption.SetValue(m_transferFunctionShaderVariantOptionName, AZ::Name("TransferFunctionType::Gamma22"));
                break;
            case TransferFunctionType::PerceptualQuantizer:
                shaderOption.SetValue(m_transferFunctionShaderVariantOptionName, AZ::Name("TransferFunctionType::PerceptualQuantizer"));
                break;
            default:
                shaderOption.SetValue(m_transferFunctionShaderVariantOptionName, AZ::Name("TransferFunctionType::None"));
                break;
            }

            UpdateShaderVariant(shaderOption);
            m_needToUpdateShaderVariant = false;
        }

        void OutputTransformPass::SetDisplayBufferFormat(RHI::Format format)
        {
            if (m_displayBufferFormat != format)
            {
                m_displayBufferFormat = format;
                if (m_displayBufferFormat == RHI::Format::R8G8B8A8_UNORM ||
                    m_displayBufferFormat == RHI::Format::B8G8R8A8_UNORM)
                {
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, OutputDeviceTransformType_48Nits);
                }
                else if (m_displayBufferFormat == RHI::Format::R10G10B10A2_UNORM)
                {
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, OutputDeviceTransformType_1000Nits);
                }
                else
                {
                    AZ_Assert(false, "Not yet supported.");
                    // To work normally on unsupported environment, initialize the display parameters by OutputDeviceTransformType_48Nits.
                    AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&m_displayMapperParameters, OutputDeviceTransformType_48Nits);
                }
            }
        }
    }   // namespace Render
}   // namespace AZ
