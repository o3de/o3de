/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/EditorModeFeedbackPassBase.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<EditorModeFeedbackPassBase> EditorModeFeedbackPassBase::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EditorModeFeedbackPassBase> pass = aznew EditorModeFeedbackPassBase(descriptor);
            return AZStd::move(pass);
        }

        EditorModeFeedbackPassBase::EditorModeFeedbackPassBase(
            const RPI::PassDescriptor& descriptor,
            const DepthTransition& depthTransition,
            float finalBlendAmount)
            : FullscreenTrianglePass(descriptor)
            , m_depthransition(depthTransition)
            , m_finalBlendAmount(finalBlendAmount)
        {
        }

        void EditorModeFeedbackPassBase::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();
            m_minDepthTransitionValueIndex.Reset();
            m_depthTransitionStartIndex.Reset();
            m_depthTransitionDurationIndex.Reset();
            m_finalBlendAmountIndex.Reset();
        }

        void EditorModeFeedbackPassBase::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();
            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void EditorModeFeedbackPassBase::SetMinDepthTransitionValue(const float minValue)
        {
            m_depthransition.m_minDepthTransitionValue = minValue;
        }

        void EditorModeFeedbackPassBase::SetDepthTransitionStart(const float start)
        {
            m_depthransition.m_depthTransitionStart = start;
        }

        void EditorModeFeedbackPassBase::SetDepthTransitionDuration(const float duration)
        {
            m_depthransition.m_depthTransitionDuration = duration;
        }

        void EditorModeFeedbackPassBase::SetFinalBlendAmount(const float amount)
        {
            m_finalBlendAmount = amount;
        }

        void EditorModeFeedbackPassBase::SetSrgConstants()
        {
            m_shaderResourceGroup->SetConstant(m_minDepthTransitionValueIndex, m_depthransition.m_minDepthTransitionValue);
            m_shaderResourceGroup->SetConstant(m_depthTransitionStartIndex, m_depthransition.m_depthTransitionStart);
            m_shaderResourceGroup->SetConstant(m_depthTransitionDurationIndex, m_depthransition.m_depthTransitionDuration);
            m_shaderResourceGroup->SetConstant(m_finalBlendAmountIndex, m_finalBlendAmount);
        }
    } // namespace Render
} // namespace AZ
