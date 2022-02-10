/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Console/IConsole.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <PostProcess/EditorModeFeedback/EditorModeFeedbackSettings.h>

#define AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, NAME, INITIAL_VALUE)                                                                           \
    AZ_CVAR(float, ##NAMESPACE##_##NAME, INITIAL_VALUE, nullptr, AZ::ConsoleFunctorFlags::Null, "");

#define AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(NAMESPACE, MIN_VALUE, START, DURATION, FINAL_BLEND)                                           \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, MinDepthTransitionValue, MIN_VALUE);                                                               \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, DepthTransitionStart, START);                                                                      \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, DepthTransitionDuration, MIN_VALUE);                                                               \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, FinalBlendAmount, FINAL_BLEND);

AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeGrayscalePass, 0.5f, 5.0f, 10.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(cl_editorModeGrayscalePass, GrayscaleAmount, 0.5f);

namespace AZ
{
    namespace Render
    {
        /**
         *  The color grading pass.
         */
        class EditorModeGrayscalePass
            : public AZ::RPI::FullscreenTrianglePass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeGrayscalePass, "{3E4FEFCB-9416-4CAE-8918-72D31AA482C5}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(EditorModeGrayscalePass, SystemAllocator, 0);

            virtual ~EditorModeGrayscalePass() = default;

            //! Creates a EditorModeGrayscalePass
            static RPI::Ptr<EditorModeGrayscalePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            EditorModeGrayscalePass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_minDepthTransitionValueIndex = "m_minDepthTransitionValue";
            RHI::ShaderInputNameIndex m_depthTransitionStartIndex = "m_depthTransitionStart";
            RHI::ShaderInputNameIndex m_depthTransitionDurationIndex = "m_depthTransitionDuration";
            RHI::ShaderInputNameIndex m_finalBlendAmountIndex = "m_finalBlendAmount";

            RHI::ShaderInputNameIndex m_grayscaleAmountIndex = "m_grayscaleAmount";
        };
    }   // namespace Render
}   // namespace AZ
