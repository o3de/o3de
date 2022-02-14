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
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

//!
#define AZ_EDITOR_MODE_PASS_CVAR(TYPE, NAMESPACE, NAME, INITIAL_VALUE)                                                                 \
    AZ_CVAR(TYPE, ##NAMESPACE##_##NAME, INITIAL_VALUE, nullptr, AZ::ConsoleFunctorFlags::Null, "");

//!
#define AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(NAMESPACE, MIN_VALUE, START, DURATION, FINAL_BLEND)                                 \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, MinDepthTransitionValue, MIN_VALUE);                                                     \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, DepthTransitionStart, START);                                                            \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, DepthTransitionDuration, DURATION);                                                     \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, FinalBlendAmount, FINAL_BLEND);

namespace AZ
{
    namespace Render
    {
        //!
        class EditorModeFeedbackPassBase
            : public AZ::RPI::FullscreenTrianglePass
        {
        public:
            AZ_RTTI(EditorModeFeedbackPassBase, "{F1F345E3-1396-47F7-9CA4-9AC87A2E9829}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(EditorModeFeedbackPassBase, SystemAllocator, 0);

            //! Creates a EditorModeFeedbackPassBase
            static RPI::Ptr<EditorModeFeedbackPassBase> Create(const RPI::PassDescriptor& descriptor);

            using AZ::RPI::FullscreenTrianglePass::FullscreenTrianglePass;

            //!
            void SetMinDepthTransitionValue(float value);

            //!
            void SetDepthTransitionStart(float value);

            //!
            void SetDepthTransitionDuration(float value);

            //!
            void SetFinalBlendAmount(float value);

        protected:

            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            //!
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_minDepthTransitionValueIndex = "m_minDepthTransitionValue";
            RHI::ShaderInputNameIndex m_depthTransitionStartIndex = "m_depthTransitionStart";
            RHI::ShaderInputNameIndex m_depthTransitionDurationIndex = "m_depthTransitionDuration";
            RHI::ShaderInputNameIndex m_finalBlendAmountIndex = "m_finalBlendAmount";

            float m_minDepthTransitionValue = 0.0f;
            float m_depthTransitionStart = 0.0f;
            float m_depthTransitionDuration = 0.0f;
            float m_finalBlendAmount = 1.0f;
        };
    } // Render
} // namespace AZ
