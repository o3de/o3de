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

// Temporary measure for configuring editor mode feedback effects at runtime until GHI 3455 is implemented
#define AZ_EDITOR_MODE_PASS_CVAR(TYPE, NAMESPACE, NAME, INITIAL_VALUE)                              \
    AZ_CVAR(TYPE, NAMESPACE##_##NAME, INITIAL_VALUE, nullptr, AZ::ConsoleFunctorFlags::Null, "");

// Temporary measure for configuring editor mode depth transitions at runtime until GHI 3455 is implemented
#define AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(NAMESPACE, MIN_VALUE, START, DURATION, FINAL_BLEND)    \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, MinDepthTransitionValue, MIN_VALUE);                 \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, DepthTransitionStart, START);                        \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, DepthTransitionDuration, DURATION);                  \
    AZ_EDITOR_MODE_PASS_CVAR(float, NAMESPACE, FinalBlendAmount, FINAL_BLEND);

namespace AZ
{
    namespace Render
    {
        //! Base class to provide depth transitioning and final blend control to all visual effect passes of the editor mode
        //! feedback system.
        class EditorModeFeedbackChildPassBase
            : public AZ::RPI::FullscreenTrianglePass
        {
        public:
            AZ_RTTI(EditorModeFeedbackChildPassBase, "{F1F345E3-1396-47F7-9CA4-9AC87A2E9829}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(EditorModeFeedbackChildPassBase, SystemAllocator);

            //! Creates a EditorModeFeedbackPassBase
            static RPI::Ptr<EditorModeFeedbackChildPassBase> Create(const RPI::PassDescriptor& descriptor);

            //! Sets the minimum blend amount that will be calculated through depth transitioning. 
            void SetMinDepthTransitionValue(float minValue);

            //! Sets the start of depth transtion for non-mask effect blending.
            void SetDepthTransitionStart(float start);

            //! Sets the duration (depth) of the depth transition band (0.0 = no depth transitioning will be applied).
            void SetDepthTransitionDuration(float duration);

            //! Sets the final blend amount that is used to scale the calculated blend values.
            void SetFinalBlendAmount(float amount);

        protected:
            using AZ::RPI::FullscreenTrianglePass::FullscreenTrianglePass;

            struct DepthTransition
            {
                //! No depth transitioning by default
                float m_minDepthTransitionValue = 0.0f;
                float m_depthTransitionStart = 0.0f;
                float m_depthTransitionDuration = 0.0f; 
            };

            EditorModeFeedbackChildPassBase(
                const RPI::PassDescriptor& descriptor, const DepthTransition& depthTransition, float finalBlendAmount);

            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            //! Sets the shader constant values for the depth transition and final blend amount for editor mode feedback effects.
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_minDepthTransitionValueIndex = "m_minDepthTransitionValue";
            RHI::ShaderInputNameIndex m_depthTransitionStartIndex = "m_depthTransitionStart";
            RHI::ShaderInputNameIndex m_depthTransitionDurationIndex = "m_depthTransitionDuration";
            RHI::ShaderInputNameIndex m_finalBlendAmountIndex = "m_finalBlendAmount";

            DepthTransition m_depthransition;
            float m_finalBlendAmount = 1.0f;
        };
    } // Render
} // namespace AZ
