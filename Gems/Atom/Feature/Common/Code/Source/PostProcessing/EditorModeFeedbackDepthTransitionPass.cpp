/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/EditorModeFeedbackDepthTransitionPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeFeedbackDepthTransitionPass> EditorModeFeedbackDepthTransitionPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeFeedbackDepthTransitionPass> pass = aznew EditorModeFeedbackDepthTransitionPass(descriptor);
            return AZStd::move(pass);
        }

        void EditorModeFeedbackDepthTransitionPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();
            m_minDepthTransitionValueIndex.Reset();
            m_depthTransitionStartIndex.Reset();
            m_depthTransitionDurationIndex.Reset();
            m_finalBlendAmountIndex.Reset();
        }

        void EditorModeFeedbackDepthTransitionPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void EditorModeFeedbackDepthTransitionPass::SetMinDepthTransitionValue(const float value)
        {
            m_minDepthTransitionValue = value;
        }

        void EditorModeFeedbackDepthTransitionPass::SetDepthTransitionStart(const float value)
        {
            m_depthTransitionStart = value;
        }

        void EditorModeFeedbackDepthTransitionPass::SetDepthTransitionDuration(const float value)
        {
            m_depthTransitionDuration = value;
        }

        void EditorModeFeedbackDepthTransitionPass::SetFinalBlendAmount(const float value)
        {
            m_finalBlendAmount = value;
        }

        void EditorModeFeedbackDepthTransitionPass::SetSrgConstants()
        {
            m_shaderResourceGroup->SetConstant(m_minDepthTransitionValueIndex, m_minDepthTransitionValue);
            m_shaderResourceGroup->SetConstant(m_depthTransitionStartIndex, m_depthTransitionStart);
            m_shaderResourceGroup->SetConstant(m_depthTransitionDurationIndex, m_depthTransitionDuration);
            m_shaderResourceGroup->SetConstant(m_finalBlendAmountIndex, m_finalBlendAmount);
        }
    } // namespace Render
} // namespace AZ
