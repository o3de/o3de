/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/EditorModeGrayscalePass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackInterface.h>

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeGrayscalePass> EditorModeGrayscalePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeGrayscalePass> pass = aznew EditorModeGrayscalePass(descriptor);
            return AZStd::move(pass);
        }
        
        EditorModeGrayscalePass::EditorModeGrayscalePass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }
        
        void EditorModeGrayscalePass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            m_minDepthTransitionValueIndex.Reset();
            m_depthTransitionStartIndex.Reset();
            m_depthTransitionDurationIndex.Reset();
            m_finalBlendAmountIndex.Reset();

            m_grayscaleAmountIndex.Reset();
        }
        
        void EditorModeGrayscalePass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        bool EditorModeGrayscalePass::IsEnabled() const
        {
            // move this to parent
            if (const auto editorModeFeedback = AZ::Interface<EditorModeFeedbackInterface>::Get())
            {
                return editorModeFeedback->IsEnabled();
            }

            return false;
        }

        void EditorModeGrayscalePass::SetSrgConstants()
        {
            m_shaderResourceGroup->SetConstant(m_minDepthTransitionValueIndex, static_cast<float>(cl_editorModeGrayscalePass_MinDepthTransitionValue));
            m_shaderResourceGroup->SetConstant(m_depthTransitionStartIndex, static_cast<float>(cl_editorModeGrayscalePass_DepthTransitionStart));
            m_shaderResourceGroup->SetConstant(m_depthTransitionDurationIndex, static_cast<float>(cl_editorModeGrayscalePass_DepthTransitionDuration));
            m_shaderResourceGroup->SetConstant(m_finalBlendAmountIndex, static_cast<float>(cl_editorModeGrayscalePass_FinalBlendAmount));

            m_shaderResourceGroup->SetConstant(m_grayscaleAmountIndex, static_cast<float>(cl_editorModeGrayscalePass_GrayscaleAmount));
        }
    }
}
