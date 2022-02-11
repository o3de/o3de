/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeTintPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackInterface.h>

AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeTintPass, 0.0f, 0.0f, 0.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(float, cl_editorModeTintPass, TintAmount, 0.5f);
AZ_EDITOR_MODE_PASS_CVAR(AZ::Color, cl_editorModeTintPass, TintColor, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f));

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeTintPass> EditorModeTintPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeTintPass> pass = aznew EditorModeTintPass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeTintPass::EditorModeTintPass(const RPI::PassDescriptor& descriptor)
            : EditorModeFeedbackDepthTransitionPass(descriptor)
        {
        }
        
        void EditorModeTintPass::InitializeInternal()
        {
            EditorModeFeedbackDepthTransitionPass::InitializeInternal();
            m_tintAmountIndex.Reset();
        }
        
        void EditorModeTintPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackDepthTransitionPass::FrameBeginInternal(params);
        }

        bool EditorModeTintPass::IsEnabled() const
        {
            // move this to parent
            if (const auto editorModeFeedback = AZ::Interface<EditorModeFeedbackInterface>::Get())
            {
                return editorModeFeedback->IsEnabled();
            }

            return false;
        }

        void EditorModeTintPass::SetTintAmount(float value)
        {
            m_tintAmount = value;
        }

        void EditorModeTintPass::SetTintColor(AZ::Color color)
        {
            m_tintColor = color;
        }

        void EditorModeTintPass::SetSrgConstants()
        {
            // THIS IS TEMP
            SetMinDepthTransitionValue(cl_editorModeTintPass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeTintPass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeTintPass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeTintPass_FinalBlendAmount);

            SetTintAmount(cl_editorModeTintPass_TintAmount);
            SetTintColor(cl_editorModeTintPass_TintColor);
            m_shaderResourceGroup->SetConstant(m_tintAmountIndex, m_tintAmount);
            m_shaderResourceGroup->SetConstant(m_tintColorIndex, m_tintColor);
        }
    }
}
