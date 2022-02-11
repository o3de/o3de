/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeBlurPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeBlurPass, 0.0f, 0.0f, 0.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(float, cl_editorModeBlurPass, KernalWidth, 1.0f);

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeBlurPass> EditorModeBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeBlurPass> pass = aznew EditorModeBlurPass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeBlurPass::EditorModeBlurPass(const RPI::PassDescriptor& descriptor)
            : EditorModeFeedbackDepthTransitionPass(descriptor)
        {
        }
        
        void EditorModeBlurPass::InitializeInternal()
        {
            EditorModeFeedbackDepthTransitionPass::InitializeInternal();
            m_kernalWidthIndex.Reset();
        }
        
        void EditorModeBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackDepthTransitionPass::FrameBeginInternal(params);
        }

        bool EditorModeBlurPass::IsEnabled() const
        {
            return true;
        }

        void EditorModeBlurPass::SetKernalWidth(float width)
        {
            m_kernalWidth = width;
        }

        void EditorModeBlurPass::SetSrgConstants()
        {
            // THIS IS TEMP
            SetMinDepthTransitionValue(cl_editorModeBlurPass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeBlurPass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeBlurPass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeBlurPass_FinalBlendAmount);

            SetKernalWidth(cl_editorModeBlurPass_KernalWidth);
            m_shaderResourceGroup->SetConstant(m_kernalWidthIndex, m_kernalWidth);
        }
    }
}
