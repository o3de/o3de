/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/EditorModeFeedbackDepthTransition.h>

namespace AZ
{
    namespace Render
    {
        void EditorModeFeedbackDepthTransition::InitializeInputNameIndices()
        {
            m_minDepthTransitionValueIndex.Reset();
            m_depthTransitionStartIndex.Reset();
            m_depthTransitionDurationIndex.Reset();
            m_finalBlendAmountIndex.Reset();
        }

        void EditorModeFeedbackDepthTransition::SetMinDepthTransitionValue(const float value)
        {
            m_minDepthTransitionValue = value;
        }

        void EditorModeFeedbackDepthTransition::SetDepthTransitionStart(const float value)
        {
            m_depthTransitionStart = value;
        }

        void EditorModeFeedbackDepthTransition::SetDepthTransitionDuration(const float value)
        {
            m_depthTransitionDuration = value;
        }

        void EditorModeFeedbackDepthTransition::SetFinalBlendAmount(const float value)
        {
            m_finalBlendAmount = value;
        }

        void EditorModeFeedbackDepthTransition::SetSrgConstants(
            Data::Instance<RPI::ShaderResourceGroup> shaderResourceGroup)
        {
            shaderResourceGroup->SetConstant(m_minDepthTransitionValueIndex, m_minDepthTransitionValue);
            shaderResourceGroup->SetConstant(m_depthTransitionStartIndex, m_depthTransitionStart);
            shaderResourceGroup->SetConstant(m_depthTransitionDurationIndex, m_depthTransitionDuration);
            shaderResourceGroup->SetConstant(m_finalBlendAmountIndex, m_finalBlendAmount);
        }
    } // namespace Render
} // namespace AZ
