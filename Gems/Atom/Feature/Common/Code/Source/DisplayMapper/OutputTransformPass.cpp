/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DisplayMapper/OutputTransformPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>

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
            : ApplyShaperLookupTablePass(descriptor)
            , m_toneMapperShaderVariantOptionName(ToneMapperShaderVariantOptionName)
            , m_transferFunctionShaderVariantOptionName(TransferFunctionShaderVariantOptionName)
            , m_ldrGradingLutShaderVariantOptionName(LdrGradingLutShaderVariantOptionName)
        {
            m_flags.m_bindViewSrg = true;
        }

        OutputTransformPass::~OutputTransformPass()
        {
        }

        void OutputTransformPass::InitializeInternal()
        {
            ApplyShaperLookupTablePass::InitializeInternal();

            AZ_Assert(m_shaderResourceGroup != nullptr, "OutputTransformPass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderInputCinemaLimitsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_cinemaLimits" });
            }

            InitializeShaderVariant();
        }

        void OutputTransformPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            if (GetLutAssetId().IsValid())
            {
                ApplyShaperLookupTablePass::SetupFrameGraphDependenciesCommon(frameGraph);
            }
            DeclareAttachmentsToFrameGraph(frameGraph);
            DeclarePassDependenciesToFrameGraph(frameGraph);

            frameGraph.SetEstimatedItemCount(1);
        }

        void OutputTransformPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "OutputTransformPass %s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            if (GetLutAssetId().IsValid())
            {
                ApplyShaperLookupTablePass::CompileResourcesCommon(context);
            }

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            CompileShaderVariant(m_shaderResourceGroup);

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

            SetSrgsForDraw(context);

            m_item.SetPipelineState(GetPipelineStateFromShaderVariant());

            commandList->Submit(m_item.GetDeviceDrawItem(context.GetDeviceIndex()));
        }

        void OutputTransformPass::FrameBeginInternal(FramePrepareParams params)
        {
            ApplyShaperLookupTablePass::FrameBeginInternal(params);
            AZ::RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                if (fp)
                {
                    AZ::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            settings->UpdateBuffer();
                            view->GetShaderResourceGroup()->SetBufferView(m_exposureControlBufferInputIndex, settings->GetBufferView());
                        }
                    }
                }
            }
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

            AZStd::vector<AZ::Name> toneMapperVariationTypes = { AZ::Name("ToneMapperType::None"),
                                                                 AZ::Name("ToneMapperType::Reinhard"),
                                                                 AZ::Name("ToneMapperType::AcesFitted"),
                                                                 AZ::Name("ToneMapperType::AcesFilmic"),
                                                                 AZ::Name("ToneMapperType::Filmic") };
            AZStd::vector<AZ::Name> transferFunctionVariationTypes = {
                AZ::Name("TransferFunctionType::None"),
                AZ::Name("TransferFunctionType::Gamma22"),
                AZ::Name("TransferFunctionType::PerceptualQuantizer") };

            AZStd::vector<AZ::Name> doLdrGradingTypes = {AZ::Name("true"), AZ::Name("false")};

            // Caching all pipeline state for each shader variation for performance reason.
            auto shaderOption = m_shader->CreateShaderOptionGroup();
            for (uint32_t tonemapperIndex = 0; tonemapperIndex < toneMapperVariationTypes.size(); ++tonemapperIndex)
            {
                for (uint32_t transferFunctionIndex = 0; transferFunctionIndex < transferFunctionVariationTypes.size(); ++transferFunctionIndex)
                {
                    for (uint32_t colorGradingIndex = 0; colorGradingIndex < doLdrGradingTypes.size(); ++colorGradingIndex)
                    {
                        shaderOption.SetValue(
                            m_toneMapperShaderVariantOptionName, toneMapperVariationTypes[tonemapperIndex]);
                        shaderOption.SetValue(
                            m_transferFunctionShaderVariantOptionName, transferFunctionVariationTypes[transferFunctionIndex]);
                        shaderOption.SetValue(
                            m_ldrGradingLutShaderVariantOptionName, doLdrGradingTypes[colorGradingIndex]);

                        PreloadShaderVariant(m_shader, shaderOption, GetRenderAttachmentConfiguration(), GetMultisampleState());
                    }
                }
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
            case ToneMapperType::AcesFitted:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::AcesFitted"));
                break;
            case ToneMapperType::Filmic:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::Filmic"));
                break;
            case ToneMapperType::AcesFilmic:
                shaderOption.SetValue(m_toneMapperShaderVariantOptionName, AZ::Name("ToneMapperType::AcesFilmic"));
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

            shaderOption.SetValue(m_ldrGradingLutShaderVariantOptionName, GetLutAssetId().IsValid() ? AZ::Name("true") : AZ::Name("false")); 

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
