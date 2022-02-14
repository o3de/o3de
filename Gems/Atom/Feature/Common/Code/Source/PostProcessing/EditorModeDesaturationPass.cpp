/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeDesaturationPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

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
            : EditorModeFeedbackPassBase(descriptor)
        {
        }
        
        void EditorModeDesaturationPass::InitializeInternal()
        {
            EditorModeFeedbackPassBase::InitializeInternal();
            m_desaturationAmountIndex.Reset();
        }
        
        void EditorModeDesaturationPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            EditorModeFeedbackPassBase::FrameBeginInternal(params);
        }

        bool EditorModeDesaturationPass::IsEnabled() const
        {
            return true;
        }

        void EditorModeDesaturationPass::SetDesaturationAmount(float value)
        {
            m_desaturationAmount = value;
        }

        void EditorModeDesaturationPass::SetSrgConstants()
        {
            // THIS IS TEMP
            SetMinDepthTransitionValue(cl_editorModeDesaturationPass_MinDepthTransitionValue);
            SetDepthTransitionStart(cl_editorModeDesaturationPass_DepthTransitionStart);
            SetDepthTransitionDuration(cl_editorModeDesaturationPass_DepthTransitionDuration);
            SetFinalBlendAmount(cl_editorModeDesaturationPass_FinalBlendAmount);

            SetDesaturationAmount(cl_editorModeDesaturationPass_DesaturationAmount);
            m_shaderResourceGroup->SetConstant(m_desaturationAmountIndex, m_desaturationAmount);
        }
    }
}
