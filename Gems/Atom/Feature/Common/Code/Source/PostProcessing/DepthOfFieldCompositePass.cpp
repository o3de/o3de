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

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/DepthOfField/DepthOfFieldSettings.h>
#include <PostProcessing/DepthOfFieldCompositePass.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DepthOfFieldCompositePass> DepthOfFieldCompositePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthOfFieldCompositePass> pass = aznew DepthOfFieldCompositePass(descriptor);
            return pass;
        }

        DepthOfFieldCompositePass::DepthOfFieldCompositePass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
            // option names from the azsl file
            , m_optionName("o_dofMode")
            , m_optionValues
            {
                AZ::Name("DepthOfFieldDebugColoring::Disabled"),
                AZ::Name("DepthOfFieldDebugColoring::Enabled")
            }
        {
        }

        void DepthOfFieldCompositePass::Init()
        {
            FullscreenTrianglePass::Init();

            AZ_Assert(m_shaderResourceGroup, "DepthOfFieldCompositePass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup)
            {
                m_backBlendFactorDivision2Index = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_backBlendFactorDivision2"));
                m_backBlendFactorDivision4Index = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_backBlendFactorDivision4"));
                m_backBlendFactorDivision8Index = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_backBlendFactorDivision8"));
                m_frontBlendFactorDivision2Index = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_frontBlendFactorDivision2"));
                m_frontBlendFactorDivision4Index = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_frontBlendFactorDivision4"));
                m_frontBlendFactorDivision8Index = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_frontBlendFactorDivision8"));
            }

            InitializeShaderVariant();
        }

        void DepthOfFieldCompositePass::InitializeShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "DepthOfFieldCompositePass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());

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

        void DepthOfFieldCompositePass::UpdateCurrentShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "DepthOfFieldCompositePass %s has a null shader when calling UpdateCurrentShaderVariant.", GetPathName().GetCStr());

            RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();

            // DebugColoring::Disabled == 0
            // DebugColoring::Enabled == 1
            int32_t index = m_enabledDebugColoring ? 1 : 0;
            shaderOption.SetValue(m_optionName, m_optionValues[index]);

            UpdateShaderVariant(shaderOption);

            m_needToUpdateShaderVariant = false;
        }

        void DepthOfFieldCompositePass::FrameBeginInternal(FramePrepareParams params)
        {
            FullscreenTrianglePass::FrameBeginInternal(params);

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    DepthOfFieldSettings* dofSettings = postProcessSettings->GetDepthOfFieldSettings();
                    if (dofSettings)
                    {
                        SetEnabledDebugColoring(dofSettings->GetEnableDebugColoring());
                        dofSettings->SetValuesToViewSrg(view->GetShaderResourceGroup());
                        m_backBlendFactorDivision2 = dofSettings->m_configurationToViewSRG.m_backBlendFactorDivision2;
                        m_backBlendFactorDivision4 = dofSettings->m_configurationToViewSRG.m_backBlendFactorDivision4;
                        m_backBlendFactorDivision8 = dofSettings->m_configurationToViewSRG.m_backBlendFactorDivision8;
                        m_frontBlendFactorDivision2 = dofSettings->m_configurationToViewSRG.m_frontBlendFactorDivision2;
                        m_frontBlendFactorDivision4 = dofSettings->m_configurationToViewSRG.m_frontBlendFactorDivision4;
                        m_frontBlendFactorDivision8 = dofSettings->m_configurationToViewSRG.m_frontBlendFactorDivision8;
                    }
                }
            }
        }

        void DepthOfFieldCompositePass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup, "DepthOfFieldCompositePass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            CompileShaderVariant(m_shaderResourceGroup);

            m_shaderResourceGroup->SetConstant(m_backBlendFactorDivision2Index, m_backBlendFactorDivision2);
            m_shaderResourceGroup->SetConstant(m_backBlendFactorDivision4Index, m_backBlendFactorDivision4);
            m_shaderResourceGroup->SetConstant(m_backBlendFactorDivision8Index, m_backBlendFactorDivision8);
            m_shaderResourceGroup->SetConstant(m_frontBlendFactorDivision2Index, m_frontBlendFactorDivision2);
            m_shaderResourceGroup->SetConstant(m_frontBlendFactorDivision4Index, m_frontBlendFactorDivision4);
            m_shaderResourceGroup->SetConstant(m_frontBlendFactorDivision8Index, m_frontBlendFactorDivision8);

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void DepthOfFieldCompositePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "DepthOfFieldCompositePass %s has a null shader resource group when calling Execute.", GetPathName().GetCStr());

            RHI::CommandList* commandList = context.GetCommandList();

            commandList->SetViewport(m_viewportState);
            commandList->SetScissor(m_scissorState);

            SetSrgsForDraw(commandList);

            m_item.m_pipelineState = GetPipelineStateFromShaderVariant();

            commandList->Submit(m_item);
        }

        void DepthOfFieldCompositePass::SetEnabledDebugColoring(bool enabled)
        {
            m_needToUpdateShaderVariant = m_needToUpdateShaderVariant || (m_enabledDebugColoring != enabled);
            m_enabledDebugColoring = enabled;
        }

    }   // namespace Render
}   // namespace AZ
