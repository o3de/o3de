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
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackInterface.h>

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

            m_minDepthTransitionValueIndex.Reset();
            m_depthTransitionStartIndex.Reset();
            m_depthTransitionDurationIndex.Reset();
            m_finalBlendAmountIndex.Reset();

            m_desaturationAmountIndex.Reset();
        }
        
        void EditorModeDesaturationPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        bool EditorModeDesaturationPass::IsEnabled() const
        {
            // move this to parent
            if (const auto editorModeFeedback = AZ::Interface<EditorModeFeedbackInterface>::Get())
            {
                return editorModeFeedback->IsEnabled();
            }

            return false;
        }

        void EditorModeDesaturationPass::SetSrgConstants()
        {
            m_shaderResourceGroup->SetConstant(m_minDepthTransitionValueIndex, static_cast<float>(cl_editorModeDesaturationPass_MinDepthTransitionValue));
            m_shaderResourceGroup->SetConstant(m_depthTransitionStartIndex, static_cast<float>(cl_editorModeDesaturationPass_DepthTransitionStart));
            m_shaderResourceGroup->SetConstant(m_depthTransitionDurationIndex, static_cast<float>(cl_editorModeDesaturationPass_DepthTransitionDuration));
            m_shaderResourceGroup->SetConstant(m_finalBlendAmountIndex, static_cast<float>(cl_editorModeDesaturationPass_FinalBlendAmount));

            m_shaderResourceGroup->SetConstant(m_desaturationAmountIndex, static_cast<float>(cl_editorModeDesaturationPass_DesaturationAmount));
        }
    }
}
