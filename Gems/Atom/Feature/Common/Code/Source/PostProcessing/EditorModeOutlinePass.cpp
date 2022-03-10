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

// Temporary measure for setting the outline pass shader parameters at runtime until GHI 3455 is implemented
AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeOutlinePass, 0.0f, 0.0f, 10.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(float, cl_editorModeOutlinePass, LineThickness, 3.0f);
AZ_EDITOR_MODE_PASS_CVAR(AZ::u8, cl_editorModeOutlinePass, LineMode, 0);
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
            : EditorModeFeedbackPassBase(descriptor, { 0.0f, 0.0f, 10.0f }, 1.0f)
        {
        }
        
        void EditorModeOutlinePass::InitializeInternal()
        {
            EditorModeFeedbackPassBase::InitializeInternal();
            m_lineThicknessIndex.Reset();
            m_lineColorIndex.Reset();
            m_lineModeIndex.Reset();
        }
        
        void EditorModeOutlinePass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackPassBase::FrameBeginInternal(params);
        }

        void EditorModeOutlinePass::SetLineThickness(const float thickness)
        {
            m_lineThickness = thickness;
        }

        void EditorModeOutlinePass::SetLineColor(AZ::Color color)
        {
            m_lineColor = AZStd::move(color);
        }

        void EditorModeOutlinePass::SetLineMode(LineMode mode)
        {
            m_lineMode = mode;
        }

        void EditorModeOutlinePass::SetSrgConstants()
        {
            // Temporary measure for setting the pass shader parameters at runtime until GHI 3455 is implemented
            SetMinDepthTransitionValue(cl_editorModeOutlinePass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeOutlinePass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeOutlinePass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeOutlinePass_FinalBlendAmount);
            SetLineThickness(cl_editorModeOutlinePass_LineThickness);
            SetLineColor(cl_editorModeOutlinePass_LineColor);
            SetLineMode(static_cast<LineMode>(AZ::u32(cl_editorModeOutlinePass_LineMode)));

            m_shaderResourceGroup->SetConstant(m_lineThicknessIndex, m_lineThickness);
            m_shaderResourceGroup->SetConstant(m_lineColorIndex, m_lineColor);
            m_shaderResourceGroup->SetConstant(m_lineModeIndex, static_cast<float>(m_lineMode));
        }
    } // namespace Render
} // namespace AZ
