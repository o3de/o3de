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
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }
        
        void EditorModeDesaturationPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();
            m_depthTransition.InitializeInputNameIndices();
            m_desaturationAmountIndex.Reset();
        }
        
        void EditorModeDesaturationPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        bool EditorModeDesaturationPass::IsEnabled() const
        {
            return true;
        }

        void EditorModeDesaturationPass::SetSrgConstants()
        {
            m_depthTransition.SetMinDepthTransitionValue(cl_editorModeDesaturationPass_MinDepthTransitionValue);
            m_depthTransition.SetDepthTransitionStart(cl_editorModeDesaturationPass_DepthTransitionStart);
            m_depthTransition.SetDepthTransitionDuration(cl_editorModeDesaturationPass_DepthTransitionDuration);
            m_depthTransition.SetFinalBlendAmount(cl_editorModeDesaturationPass_FinalBlendAmount);

            m_depthTransition.SetSrgConstants(m_shaderResourceGroup);
            m_shaderResourceGroup->SetConstant(m_desaturationAmountIndex, static_cast<float>(cl_editorModeDesaturationPass_DesaturationAmount));
        }
    }
}
