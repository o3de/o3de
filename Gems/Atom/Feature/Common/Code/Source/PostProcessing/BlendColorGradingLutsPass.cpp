/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/BlendColorGradingLutsPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        static const char* const NumSourceLutsShaderVariantOptionName{ "o_numSourceLuts" };
        
        RPI::Ptr<BlendColorGradingLutsPass> BlendColorGradingLutsPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<BlendColorGradingLutsPass> pass = aznew BlendColorGradingLutsPass(descriptor);
            return AZStd::move(pass);
        }

        BlendColorGradingLutsPass::BlendColorGradingLutsPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
            , m_numSourceLutsShaderVariantOptionName(NumSourceLutsShaderVariantOptionName)
        {
        }

        BlendColorGradingLutsPass::~BlendColorGradingLutsPass()
        {
            ReleaseLutImage();
        }

        void BlendColorGradingLutsPass::InitializeInternal()
        {
            InitializeShaderVariant();
        }

        void BlendColorGradingLutsPass::InitializeShaderVariant()
        {
            if (m_shader == nullptr)
            {
                AZ_Assert(false, "BlendColorGradingLutsPass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());
                return;
            }

            // Total variations is MaxBlendLuts plus one for the fallback case that none of the LUTs are found,
            // and hence zero LUTs are blended resulting in an identity LUT.
            uint32_t totalVariantCount = static_cast<uint32_t>(LookModificationSettings::MaxBlendLuts) + 1;

            // Caching all pipeline state for each shader variation for performance reason.
            for (uint32_t shaderVariantIndex = 0; shaderVariantIndex < totalVariantCount; ++shaderVariantIndex)
            {
                auto shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_numSourceLutsShaderVariantOptionName, RPI::ShaderOptionValue{ shaderVariantIndex });

                RPI::ShaderVariant shaderVariant = m_shader->GetVariant(shaderOption.GetShaderVariantId());

                RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shaderOption);

                ShaderVariantInfo variantInfo{
                    !shaderVariant.UseKeyFallback(),
                    m_shader->AcquirePipelineState(pipelineStateDescriptor)
                };
                m_shaderVariant.push_back(AZStd::move(variantInfo));
            }

            m_needToUpdateShaderVariant = true;
        }

        void BlendColorGradingLutsPass::UpdateCurrentShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "BlendColorGradingLutsPass %s has a null shader when calling UpdateCurrentShaderVariant.", GetPathName().GetCStr());
            if (m_numSourceLuts > LookModificationSettings::MaxBlendLuts)
            {
                // Invalid number of LUTs. Fallback to generate the identity LUT.
                AZ_Assert(false, "BlendColorGradingLutsPass %s has an invalid number of LUTs for blending (%u).", GetPathName().GetCStr(), m_numSourceLuts);
                m_numSourceLuts = 0;
            }
            else
            {
                m_currentShaderVariantIndex = m_numSourceLuts;
            }

            if (!m_shaderVariant[m_currentShaderVariantIndex].m_isFullyBaked)
            {
                auto shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_numSourceLutsShaderVariantOptionName, RPI::ShaderOptionValue{ m_numSourceLuts });
                m_currentShaderVariantKeyFallbackValue = shaderOption.GetShaderVariantKeyFallbackValue();
            }
            m_needToUpdateShaderVariant = false;
        }

        void BlendColorGradingLutsPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            ComputePass::SetupFrameGraphDependencies(frameGraph);

            CheckLutBlendSettings();

            if (m_needToUpdateLut && m_blendedLut.m_lutImage == nullptr)
            {
                AcquireLutImage();
            }

            if (m_blendedLut.m_lutImage == nullptr)
            {
                return;
            }

            AZ_Assert(m_blendedLut.m_lutImage != nullptr, "BlendColorGradingLutsPass unable to acquire LUT image");

            AZ::RHI::AttachmentId imageAttachmentId = AZ::RHI::AttachmentId("BlendColorGradingLutImageAttachmentId");

            // import this attachment if it wasn't imported
            if (!frameGraph.GetAttachmentDatabase().IsAttachmentValid(imageAttachmentId))
            {
                [[maybe_unused]] RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportImage(imageAttachmentId, m_blendedLut.m_lutImage.get());
                AZ_Error("BlendColorGradingLutsPass", result == RHI::ResultCode::Success, "Failed to import BlendColorGradingLutImageAttachmentId with error %d", result);
            }

            RHI::ImageScopeAttachmentDescriptor desc;
            desc.m_attachmentId = imageAttachmentId;
            desc.m_imageViewDescriptor = m_blendedLut.m_lutImageViewDescriptor;
            desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

            frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
        }

        void BlendColorGradingLutsPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "BlendColorGradingLutsPass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            // Early out if no LUT is needed
            if (m_blendedLut.m_lutImage == nullptr)
            {
                return;
            }

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderResourceGroup->SetImageView(m_shaderInputBlendedLutImageIndex, m_blendedLut.m_lutImageView.get());
                m_shaderResourceGroup->SetConstant(m_shaderInputBlendedLutDimensionsIndex, m_blendedLutDimensions);
                m_shaderResourceGroup->SetConstant(m_shaderInputBlendedLutShaperTypeIndex, m_blendedLutShaperParams.m_type);
                m_shaderResourceGroup->SetConstant(m_shaderInputBlendededLutShaperBiasIndex, m_blendedLutShaperParams.m_bias);
                m_shaderResourceGroup->SetConstant(m_shaderInputBlendededLutShaperScaleIndex, m_blendedLutShaperParams.m_scale);
                m_shaderResourceGroup->SetConstant(m_shaderInputWeight0Index, m_weights[0]);
                m_shaderResourceGroup->SetConstant(m_shaderInputWeight1Index, m_weights[1]);
                m_shaderResourceGroup->SetConstant(m_shaderInputWeight2Index, m_weights[2]);
                m_shaderResourceGroup->SetConstant(m_shaderInputWeight3Index, m_weights[3]);
                m_shaderResourceGroup->SetConstant(m_shaderInputWeight4Index, m_weights[4]);

                if (m_colorGradingLuts[0].m_lutStreamingImage)
                {
                    m_shaderResourceGroup->SetImageView(m_shaderInputSourceLut1ImageIndex, m_colorGradingLuts[0].m_lutStreamingImage->GetImageView());
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut1ShaperTypeIndex, m_colorGradingShaperParams[0].m_type);
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut1ShaperBiasIndex, m_colorGradingShaperParams[0].m_bias);
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut1ShaperScaleIndex, m_colorGradingShaperParams[0].m_scale);
                }

                if (m_colorGradingLuts[1].m_lutStreamingImage)
                {
                    m_shaderResourceGroup->SetImageView(m_shaderInputSourceLut2ImageIndex, m_colorGradingLuts[1].m_lutStreamingImage->GetImageView());
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut2ShaperTypeIndex, m_colorGradingShaperParams[1].m_type);
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut2ShaperBiasIndex, m_colorGradingShaperParams[1].m_bias);
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut2ShaperScaleIndex, m_colorGradingShaperParams[1].m_scale);
                }

                if (m_colorGradingLuts[2].m_lutStreamingImage)
                {
                    m_shaderResourceGroup->SetImageView(m_shaderInputSourceLut3ImageIndex, m_colorGradingLuts[2].m_lutStreamingImage->GetImageView());
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut3ShaperTypeIndex, m_colorGradingShaperParams[2].m_type);
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut3ShaperBiasIndex, m_colorGradingShaperParams[2].m_bias);
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut3ShaperScaleIndex, m_colorGradingShaperParams[2].m_scale);
                }

                if (m_colorGradingLuts[3].m_lutStreamingImage)
                {
                    m_shaderResourceGroup->SetImageView(m_shaderInputSourceLut4ImageIndex, m_colorGradingLuts[3].m_lutStreamingImage->GetImageView());
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut4ShaperTypeIndex, m_colorGradingShaperParams[3].m_type);
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut4ShaperBiasIndex, m_colorGradingShaperParams[3].m_bias);
                    m_shaderResourceGroup->SetConstant(m_shaderInputSourceLut4ShaperScaleIndex, m_colorGradingShaperParams[3].m_scale);
                }

                if (!m_shaderVariant[m_currentShaderVariantIndex].m_isFullyBaked && m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
                {
                    m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(m_currentShaderVariantKeyFallbackValue);
                }
            }

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void BlendColorGradingLutsPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            if (m_needToUpdateLut && m_blendedLut.m_lutImage && m_currentShaderVariantIndex <= LookModificationSettings::MaxBlendLuts)
            {
                m_dispatchItem.SetPipelineState(m_shaderVariant[m_currentShaderVariantIndex].m_pipelineState);

                ComputePass::BuildCommandListInternal(context);

                m_needToUpdateLut = false;
            }
        }

        void BlendColorGradingLutsPass::AcquireLutImage()
        {
            auto displayMapper = m_pipeline->GetScene()->GetFeatureProcessor<AZ::Render::AcesDisplayMapperFeatureProcessor>();

            if (displayMapper)
            {
                displayMapper->GetOwnedLut(m_blendedLut, AZ::Name("ColorGradingBlendedLut"));
                // Update dimensions
                if (m_blendedLut.m_lutImage)
                {
                    const AZ::RHI::ImageDescriptor& desc = m_blendedLut.m_lutImage->GetDescriptor();
                    m_blendedLutDimensions[0] = desc.m_size.m_width;
                    m_blendedLutDimensions[1] = desc.m_size.m_height;
                    m_blendedLutDimensions[2] = desc.m_size.m_depth;
                }
            }
        }

        void BlendColorGradingLutsPass::ReleaseLutImage()
        {
            m_blendedLut.m_lutImage.reset();
            m_blendedLut.m_lutImageView.reset();
            m_blendedLut = {};
        }

        void BlendColorGradingLutsPass::SetShaperParameters(const ShaperParams& shaperParams)
        {
            m_blendedLutShaperParams = shaperParams;
        }

        
        AZStd::optional<ShaperParams> BlendColorGradingLutsPass::GetCommonShaperParams() const
        {
            LookModificationSettings* settings = GetLookModificationSettings();
            if (settings)
            {
                settings->PrepareLutBlending();

                ShaperPresetType type = ShaperPresetType::NumShaperTypes;
                float customMinExposure = 0.0;
                float customMaxExposure = 0.0;

                for (size_t lutIndex = 0; lutIndex < settings->GetLutBlendStackSize(); lutIndex++)
                {
                    LutBlendItem& lutBlendItem = settings->GetLutBlendItem(lutIndex);

                    if (lutIndex == 0)
                    {
                        type = lutBlendItem.m_shaperPreset;
                        customMinExposure = lutBlendItem.m_customMinExposure;
                        customMaxExposure = lutBlendItem.m_customMaxExposure;
                    }
                    else if (type != lutBlendItem.m_shaperPreset)
                    {
                        // Shapers are different
                        return AZStd::nullopt;
                    }
                    else if (type == ShaperPresetType::LinearCustomRange || type == ShaperPresetType::Log2CustomRange)
                    {
                        if (lutBlendItem.m_customMinExposure != customMinExposure ||
                            lutBlendItem.m_customMaxExposure != customMaxExposure)
                        {
                            // Shapers are same, but custom exposure for custom type is different.
                            return AZStd::nullopt;
                        }
                    }
                }

                // Only calculate shaper params when there's at least one lut blend.
                if (settings->GetLutBlendStackSize() > 0)
                {
                    return AcesDisplayMapperFeatureProcessor::GetShaperParameters(type, customMinExposure, customMaxExposure);
                }
            }
            return AZStd::nullopt;
        }

        void BlendColorGradingLutsPass::CheckLutBlendSettings()
        {
            LookModificationSettings* settings = GetLookModificationSettings();
            if (settings)
            {
                settings->PrepareLutBlending();

                // Early out if the settings have not chanced
                HashValue64 hash = settings->GetHash();
                if (hash == m_lutBlendHash)
                {
                    return;
                }
                m_lutBlendHash = hash;

                m_needToUpdateLut = true;

                // Calculate all the weights and LUT assets and check if there has been a change
                // Only the top N LUTs will be blended where N = LookModificationSettings::MaxBlendLuts
                // Weight 0 is used for the base color, and the other weights are for the LUTs in increasing priority
                size_t numLuts = settings->GetLutBlendStackSize();

                float intensity[LookModificationSettings::MaxBlendLuts];
                float one_intensity[LookModificationSettings::MaxBlendLuts];
                float over[LookModificationSettings::MaxBlendLuts];
                float one_over[LookModificationSettings::MaxBlendLuts];

                for (int curLutIndex = 0; curLutIndex < LookModificationSettings::MaxBlendLuts; curLutIndex++)
                {
                    intensity[curLutIndex] = 0.f;
                    one_intensity[curLutIndex] = 1.f;
                    over[curLutIndex] = 0.f;
                    one_over[curLutIndex] = 1.f;
                }

                uint32_t current = 0;
                for (size_t lutIndex = 0; lutIndex < numLuts; lutIndex++)
                {
                    LutBlendItem& lutBlendItem = settings->GetLutBlendItem(lutIndex);
                    const auto assetId = lutBlendItem.m_asset.GetId();

                    if (assetId.IsValid())
                    {
                        AcesDisplayMapperFeatureProcessor* dmfp = GetScene()->GetFeatureProcessor<AcesDisplayMapperFeatureProcessor>();
                        dmfp->GetLutFromAssetId(m_colorGradingLuts[current], assetId);
                        if (!m_colorGradingLuts[current].m_lutStreamingImage)
                        {
                            AZ_Warning("BlendColorGradingLutsPass", false, "Unable to load grading LUT from asset %s",
                                lutBlendItem.m_asset.ToString<AZStd::string>().c_str());
                            // Skip this LUT
                            continue;
                        }
                    }

                    intensity[current] = lutBlendItem.m_intensity;
                    one_intensity[current] = 1.0f - lutBlendItem.m_intensity;
                    over[current] = lutBlendItem.m_overrideStrength;
                    one_over[current] = 1.0f - lutBlendItem.m_overrideStrength;

                    m_colorGradingShaperParams[current] = AcesDisplayMapperFeatureProcessor::GetShaperParameters(
                        lutBlendItem.m_shaperPreset,
                        lutBlendItem.m_customMinExposure,
                        lutBlendItem.m_customMaxExposure
                    );

                    ++current;
                    if (current == LookModificationSettings::MaxBlendLuts)
                    {
                        break;
                    }
                }

                m_weights[0] = 0.f;
                // Handle the case where there are no LUTs to be blended, and hence an identity LUT will be generated
                if (current == 0)
                {
                    m_weights[0] = 1.f;
                    // These weights would not be used in the shader in this case, but setting to zero anyways.
                    for (int lutIndex = 1; lutIndex < LookModificationSettings::MaxBlendLuts + 1; lutIndex++)
                    {
                        m_weights[lutIndex] = 0.f;
                    }
                }
                else
                {
                    // Compute all the weights
                    // First compute the weight of the ungraded color value
                    for (uint32_t lutIndex = 0; lutIndex < current; lutIndex++)
                    {
                        float weight = one_intensity[lutIndex] * over[lutIndex];
                        for (int overrideLutIndex = lutIndex + 1; overrideLutIndex < LookModificationSettings::MaxBlendLuts; overrideLutIndex++)
                        {
                            weight *= one_over[overrideLutIndex];
                        }
                        m_weights[0] += weight;
                    }
                    // Then compute the weights for the LUTs
                    for (uint32_t weightIndex = 0; weightIndex < current; weightIndex++)
                    {
                        m_weights[weightIndex + 1] = intensity[weightIndex] * over[weightIndex];
                        for (int lutIndex = weightIndex + 1; lutIndex < LookModificationSettings::MaxBlendLuts; lutIndex++)
                        {
                            m_weights[weightIndex + 1] *= one_over[lutIndex];
                        }
                    }
                }

                // If the number of source LUTs have changed, the shader variant will need to be updated
                if (m_numSourceLuts != current)
                {
                    m_numSourceLuts = current;
                    m_needToUpdateShaderVariant = true;
                }
            }
        }

        LookModificationSettings* BlendColorGradingLutsPass::GetLookModificationSettings() const
        {
            AZ::RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        LookModificationSettings* settings = postProcessSettings->GetLookModificationSettings();
                        if (settings)
                        {
                            return settings;
                        }
                    }
                }
            }
            return nullptr;
        }
    }   // namespace Render
}   // namespace AZ
