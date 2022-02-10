/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/IConsole.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

//!
#define AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, NAME, INITIAL_VALUE)                                                                 \
    AZ_CVAR(float, ##NAMESPACE##_##NAME, INITIAL_VALUE, nullptr, AZ::ConsoleFunctorFlags::Null, "");

//!
#define AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(NAMESPACE, MIN_VALUE, START, DURATION, FINAL_BLEND)                                 \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, MinDepthTransitionValue, MIN_VALUE);                                                     \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, DepthTransitionStart, START);                                                            \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, DepthTransitionDuration, MIN_VALUE);                                                     \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, FinalBlendAmount, FINAL_BLEND);

namespace AZ
{
    namespace Render
    {
        //!
        class EditorModeFeedbackDepthTransition
        {
        public:
            //!
            void InitializeInputNameIndices();

            //!
            void SetSrgConstants(Data::Instance<RPI::ShaderResourceGroup> shaderResourceGroup);

            //!
            void SetMinDepthTransitionValue(float value);

            //!
            void SetDepthTransitionStart(float value);

            //!
            void SetDepthTransitionDuration(float value);

            //!
            void SetFinalBlendAmount(float value);

        private:
            RHI::ShaderInputNameIndex m_minDepthTransitionValueIndex = "m_minDepthTransitionValue";
            RHI::ShaderInputNameIndex m_depthTransitionStartIndex = "m_depthTransitionStart";
            RHI::ShaderInputNameIndex m_depthTransitionDurationIndex = "m_depthTransitionDuration";
            RHI::ShaderInputNameIndex m_finalBlendAmountIndex = "m_finalBlendAmount";

            float m_minDepthTransitionValue;
            float m_depthTransitionStart;
            float m_depthTransitionDuration;
            float m_finalBlendAmount;
        };
    } // Render
} // namespace AZ
