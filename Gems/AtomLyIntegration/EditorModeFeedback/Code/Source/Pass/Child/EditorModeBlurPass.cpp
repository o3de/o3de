/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <Pass/Child/EditorModeBlurPass.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

// Temporary measure for setting the blur pass shader parameters at runtime until GHI 3455 is implemented
AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeBlurPass, 0.0f, 0.0f, 20.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(float, cl_editorModeBlurPass, KernelHalfWidth, 5.0f);

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
            : EditorModeFeedbackChildPassBase(descriptor, { 0.0f, 0.0f, 20.0f }, 1.0f)
        {
        }
        
        void EditorModeBlurPass::InitializeInternal()
        {
            EditorModeFeedbackChildPassBase::InitializeInternal();
            m_kernelHalfWidthIndex.Reset();
        }
        
        void EditorModeBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackChildPassBase::FrameBeginInternal(params);
        }

        void EditorModeBlurPass::SetKernelHalfWidth(const float width)
        {
            m_kernelHalfWidth = width;
        }

        void EditorModeBlurPass::SetSrgConstants()
        {
            // Temporary measure for setting the pass shader parameters at runtime until GHI 3455 is implemented
            SetMinDepthTransitionValue(cl_editorModeBlurPass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeBlurPass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeBlurPass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeBlurPass_FinalBlendAmount);
            SetKernelHalfWidth(cl_editorModeBlurPass_KernelHalfWidth);

            m_shaderResourceGroup->SetConstant(m_kernelHalfWidthIndex, m_kernelHalfWidth);
        }
    } // namespace Render
} // namespace AZ
