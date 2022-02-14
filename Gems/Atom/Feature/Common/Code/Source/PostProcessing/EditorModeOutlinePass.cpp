/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeOutlinePass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackInterface.h>

AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeOutlinePass, 0.0f, 0.0f, 1000.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(float, cl_editorModeOutlinePass, LineThickness, 3.0f);
AZ_EDITOR_MODE_PASS_CVAR(AZ::Color, cl_editorModeOutlinePass, LineColor, AZ::Color(0.96f, 0.65f, 0.13f, 1.0f));

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeOutlinePass> EditorModeOutlinePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeOutlinePass> pass = aznew EditorModeOutlinePass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeOutlinePass::EditorModeOutlinePass(const RPI::PassDescriptor& descriptor)
            : EditorModeFeedbackPassBase(descriptor)
        {
        }
        
        void EditorModeOutlinePass::InitializeInternal()
        {
            EditorModeFeedbackPassBase::InitializeInternal();
            m_lineThicknessIndex.Reset();
            m_lineColorIndex.Reset();
        }
        
        void EditorModeOutlinePass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackPassBase::FrameBeginInternal(params);
        }

        bool EditorModeOutlinePass::IsEnabled() const
        {
            return true;
        }

        void EditorModeOutlinePass::SetLineThickness(float width)
        {
            m_lineThickness = width;
        }

        void EditorModeOutlinePass::SetLineColor(AZ::Color color)
        {
            m_lineColor = color;
        }

        void EditorModeOutlinePass::SetSrgConstants()
        {
            // THIS IS TEMP
            SetMinDepthTransitionValue(cl_editorModeOutlinePass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeOutlinePass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeOutlinePass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeOutlinePass_FinalBlendAmount);

            SetLineThickness(cl_editorModeOutlinePass_LineThickness);
            SetLineColor(cl_editorModeOutlinePass_LineColor);
            m_shaderResourceGroup->SetConstant(m_lineThicknessIndex, m_lineThickness);
            m_shaderResourceGroup->SetConstant(m_lineColorIndex, m_lineColor);
        }
    }
}
