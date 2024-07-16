/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/DepthOfField/DepthOfFieldSettings.h>
#include <PostProcessing/DepthOfFieldPencilMap.h>
#include <PostProcessing/DepthOfFieldBokehBlurPass.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DepthOfFieldBokehBlurPass> DepthOfFieldBokehBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthOfFieldBokehBlurPass> pass = aznew DepthOfFieldBokehBlurPass(descriptor);
            return pass;
        }

        DepthOfFieldBokehBlurPass::DepthOfFieldBokehBlurPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
            // option names from the azsl file
            , m_optionName("o_sampleNumber")
            , m_optionValues
            {
                AZ::Name("SampleNumber::Sample6"),
                AZ::Name("SampleNumber::Sample18"),
                AZ::Name("SampleNumber::Sample36"),
                AZ::Name("SampleNumber::Sample60")
            }
        {
        }

        void DepthOfFieldBokehBlurPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            m_sampleNumberIndex.Reset();
            m_radiusMinIndex.Reset();
            m_radiusMaxIndex.Reset();
            m_sampleTexcoordsRadiusIndex.Reset();

            InitializeShaderVariant();
        }

        void DepthOfFieldBokehBlurPass::InitializeShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "DepthOfFieldBokehBlurPass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());

            // Caching all pipeline state for each shader variation for performance reason.
            auto optionValueCount = m_optionValues.size();
            for (auto shaderVariantIndex = 0; shaderVariantIndex < optionValueCount; ++shaderVariantIndex)
            {
                auto shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_optionName, m_optionValues[shaderVariantIndex]);
                PreloadShaderVariant(m_shader, shaderOption, GetRenderAttachmentConfiguration(), GetMultisampleState());
            }

            m_needToUpdateShaderVariant = true;
        }

        void DepthOfFieldBokehBlurPass::UpdateCurrentShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "DepthOfFieldBokehBlurPass %s has a null shader when calling UpdateCurrentShaderVariant.", GetPathName().GetCStr());

            // Sample6 == 0
            // Sample18 == 1
            // Sample36 == 2
            // Sample60 == 3
            int index = 0;
            switch (m_sampleNumber)
            {
            case 6:
                index = 0;
                break;
            case 18:
                index = 1;
                break;
            case 36:
                index = 2;
                break;
            case 60:
                index = 3;
                break;
            default:
                AZ_Assert(false, "%s : sampleNumber is illegal when calling Compile.", GetPathName().GetCStr());
                break;
            }

            RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
            shaderOption.SetValue(m_optionName, m_optionValues[index]);
            UpdateShaderVariant(shaderOption);

            m_needToUpdateShaderVariant = false;
        }

        void DepthOfFieldBokehBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    DepthOfFieldSettings* dofSettings = postProcessSettings->GetDepthOfFieldSettings();
                    if (dofSettings)
                    {
                        AZ::RHI::Handle<uint32_t> splitSize = dofSettings->GetSplitSizeForPass(GetName());

                        if (splitSize.IsValid())
                        {
                            switch (splitSize.GetIndex())
                            {
                            case 2:
                                UpdateSampleTexcoords(dofSettings->m_sampleRadialDivision2, dofSettings->m_viewAspectRatio);
                                SetRadiusMinMax(dofSettings->m_minBokehRadiusDivision2, dofSettings->m_maxBokehRadiusDivision2);
                                break;
                            case 4:
                                UpdateSampleTexcoords(dofSettings->m_sampleRadialDivision4, dofSettings->m_viewAspectRatio);
                                SetRadiusMinMax(dofSettings->m_minBokehRadiusDivision4, dofSettings->m_maxBokehRadiusDivision4);
                                break;
                            case 8:
                                UpdateSampleTexcoords(dofSettings->m_sampleRadialDivision8, dofSettings->m_viewAspectRatio);
                                SetRadiusMinMax(dofSettings->m_minBokehRadiusDivision8, dofSettings->m_maxBokehRadiusDivision8);
                                break;
                            default:
                                AZ_Assert(false, "DepthOfFieldBokehBlurPass : Failed to get the division number from pass request name for blur.");
                                break;
                            }
                        }
                    }
                }
            }
            RPI::FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void DepthOfFieldBokehBlurPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup, "DepthOfFieldBokehBlurPass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            CompileShaderVariant(m_shaderResourceGroup);

            m_shaderResourceGroup->SetConstant(m_radiusMinIndex, m_radiusMin);
            m_shaderResourceGroup->SetConstant(m_radiusMaxIndex, m_radiusMax);
            m_shaderResourceGroup->SetConstantArray(m_sampleTexcoordsRadiusIndex, m_sampleTexcoords);

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void DepthOfFieldBokehBlurPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "DepthOfFieldBokehBlurPass %s has a null shader resource group when calling Execute.", GetPathName().GetCStr());

            RHI::CommandList* commandList = context.GetCommandList();

            commandList->SetViewport(m_viewportState);
            commandList->SetScissor(m_scissorState);

            SetSrgsForDraw(context);

            m_item.SetPipelineState(GetPipelineStateFromShaderVariant());

            commandList->Submit(m_item.GetDeviceDrawItem(context.GetDeviceIndex()));
        }

        void DepthOfFieldBokehBlurPass::SetRadiusMinMax(float min, float max)
        {
            m_radiusMin = min;
            m_radiusMax = max;
        }

        void DepthOfFieldBokehBlurPass::UpdateSampleTexcoords(uint32_t radialDivisionCount, float viewAspectRatio)
        {
            // calculate sampling texcoords.
            // sample 6 points around center pixel.
            // Similarly, sample around it as 12,18,24 points.

            AZ_Assert(radialDivisionCount >= 1 && radialDivisionCount <= 4, "DepthOfFieldBokehBlurPass : radialDivisionCount is illegal value.");

            // radialDivisionCount | sampleNumber     |
            //         1           | 6                |
            //         2           | 6 + 12           |
            //         3           | 6 + 12 + 18      |
            //         4           | 6 + 12 + 18 + 24 |
            uint32_t newSampleNumber = 3 * radialDivisionCount * (radialDivisionCount + 1);

            // Switch shader variant when value changes.
            m_needToUpdateShaderVariant = m_needToUpdateShaderVariant || (m_sampleNumber != newSampleNumber);
            m_sampleNumber = newSampleNumber;

            AZ_Assert(m_sampleNumber <= SampleMax, "DepthOfFieldBokehBlurPass : sampleNumber is over.");

            // It is possible that copying this whole function in shader, may execute faster than passing the baked result as a memory-bound resource.
            // Doing the verification depends on how much we want to invest in optimization investigation.
            for (uint32_t index = 0; index < m_sampleNumber; index++)
            {
                // index[ 0- 5] -> radiusStep[1], angleStep[0- 5]
                // index[ 6-17] -> radiusStep[2], angleStep[0-11]
                // index[18-35] -> radiusStep[3], angleStep[0-17]
                // index[36-59] -> radiusStep[4], angleStep[0-23]
                uint32_t radiusStep = 0;
                if (index < 6)
                {
                    radiusStep = 1;
                }
                else if (index < 18)
                {
                    radiusStep = 2;
                }
                else if (index < 36)
                {
                    radiusStep = 3;
                }
                else if (index < 60)
                {
                    radiusStep = 4;
                }
                else
                {
                    AZ_Assert(false, "DepthOfFieldBokehBlurPass : index is illegal.");
                }
                uint32_t angleStep = index - 3 * radiusStep * (radiusStep - 1);
                AZ_Assert(radiusStep >= 1 && radiusStep <= 4, "DepthOfFieldBokehBlurPass : radiusStep is illegal value.");

                // Divide by 'sampleRadial + 1' instead of 'sampleRadial' to shift the sampling inwards and increase useful sampling.
                float radius = static_cast<float>(radiusStep) / static_cast<float>(radialDivisionCount + 1);

                constexpr float AngleOffset = 0.5f;
                float angle = static_cast<float>(angleStep + AngleOffset) * (2.0f * AZ::Constants::Pi) / static_cast<float>(radius * 6);

                // The coordinates of the color buffer to be sampled.
                float colorTexcoordU = cosf(angle) * radius;
                float colorTexcoordV = sinf(angle) * radius;
                colorTexcoordU /= viewAspectRatio;

                m_sampleTexcoords[index][0] = colorTexcoordU;
                m_sampleTexcoords[index][1] = colorTexcoordV;

                // pencilmap
                // texcoord U is calculated based on depth in shader. It is not calculated here.
                // texcoord V is based on radius. The coordinate is radius 0 at the bottom and radius 1 at the top.
                float pencilmapTexcoordV = 1.0f - radius;
                m_sampleTexcoords[index][2] = pencilmapTexcoordV;

                m_sampleTexcoords[index][3] = 0.0f;
            }
        }

    }   // namespace Render
}   // namespace AZ
