/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <Pass/Child/EditorModeDesaturationPass.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

// Temporary measure for setting the desaturation pass shader parameters at runtime until GHI 3455 is implemented
AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeDesaturationPass, 0.75f, 0.0f, 20.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(float, cl_editorModeDesaturationPass, DesaturationAmount, 1.0f);

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeDesaturationPass> EditorModeDesaturationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeDesaturationPass> pass = aznew EditorModeDesaturationPass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeDesaturationPass::EditorModeDesaturationPass(const RPI::PassDescriptor& descriptor)
            : EditorModeFeedbackChildPassBase(descriptor, { 0.75f, 0.0f, 20.0f }, 1.0f)
        {
        }
        
        void EditorModeDesaturationPass::InitializeInternal()
        {
            EditorModeFeedbackChildPassBase::InitializeInternal();
            m_desaturationAmountIndex.Reset();
        }
        
        void EditorModeDesaturationPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackChildPassBase::FrameBeginInternal(params);
        }
        
        void EditorModeDesaturationPass::SetDesaturationAmount(const float amount)
        {
            m_desaturationAmount = amount;
        }

        void EditorModeDesaturationPass::SetSrgConstants()
        {
            // Temporary measure for setting the pass shader parameters at runtime until GHI 3455 is implemented
            SetMinDepthTransitionValue(cl_editorModeDesaturationPass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeDesaturationPass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeDesaturationPass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeDesaturationPass_FinalBlendAmount);
            SetDesaturationAmount(cl_editorModeDesaturationPass_DesaturationAmount);

            m_shaderResourceGroup->SetConstant(m_desaturationAmountIndex, m_desaturationAmount);
        }
    } // namespace Render
} // namespace AZ
