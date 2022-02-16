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

// Temporary measure for setting the blur pass shader parameters at runtime until LYN-5294 is implemented
AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeBlurPass, 0.0f, 0.0f, 20.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(float, cl_editorModeBlurPass, KernalWidth, 5.0f);

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
            : EditorModeFeedbackPassBase(descriptor, { 0.0f, 0.0f, 20.0f }, 1.0f)
        {
        }
        
        void EditorModeBlurPass::InitializeInternal()
        {
            EditorModeFeedbackPassBase::InitializeInternal();
            m_kernalWidthIndex.Reset();
        }
        
        void EditorModeBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackPassBase::FrameBeginInternal(params);
        }

        bool EditorModeBlurPass::IsEnabled() const
        {
            return true;
        }

        void EditorModeBlurPass::SetKernalWidth(const float width)
        {
            m_kernalWidth = width;
        }

        void EditorModeBlurPass::SetSrgConstants()
        {
            // Temporary measure for setting the pass shader parameters at runtime until LYN-5294 is implemented
            SetMinDepthTransitionValue(cl_editorModeBlurPass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeBlurPass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeBlurPass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeBlurPass_FinalBlendAmount);
            SetKernalWidth(cl_editorModeBlurPass_KernalWidth);

            m_shaderResourceGroup->SetConstant(m_kernalWidthIndex, m_kernalWidth);
        }
    } // namespace Render
} // namespace AZ
