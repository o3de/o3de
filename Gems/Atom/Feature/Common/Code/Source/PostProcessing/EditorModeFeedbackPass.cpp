/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeFeedbackPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeFeedbackPass> EditorModeFeedbackPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeFeedbackPass> pass = aznew EditorModeFeedbackPass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeFeedbackPass::EditorModeFeedbackPass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }
        
        void EditorModeFeedbackPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            m_desaturationAmountIndex.Reset();
        }
        
        void EditorModeFeedbackPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        bool EditorModeFeedbackPass::IsEnabled() const
        {
            //const auto* editorModeFeedbackSettings = GetEditorModeFeedbackSettings();
            //return editorModeFeedbackSettings ? editorModeFeedbackSettings->GetEnabled() : false;
            return true;
        }

        void EditorModeFeedbackPass::SetSrgConstants()
        {
            const EditorModeFeedbackSettings* settings = GetEditorModeFeedbackSettings();
            if (settings)
            {
                m_shaderResourceGroup->SetConstant(m_desaturationAmountIndex, settings->GetDesaturationAmount());
            }
        }

        const AZ::Render::EditorModeFeedbackSettings* EditorModeFeedbackPass::GetEditorModeFeedbackSettings() const
        {
            RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        const EditorModeFeedbackSettings* editorModeFeedbackSettings = postProcessSettings->GetEditorModeFeedbackSettings();
                        if (editorModeFeedbackSettings != nullptr/* && editorModeFeedbackSettings->GetEnabled()*/)
                        {
                            return postProcessSettings->GetEditorModeFeedbackSettings();
                        }
                    }
                }
            }
            return nullptr;
        }
    }
}
