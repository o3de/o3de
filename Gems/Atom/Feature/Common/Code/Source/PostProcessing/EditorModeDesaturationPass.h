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

#define AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(NAMESPACE, MIN_VALUE, START, DURATION)                                                        \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, MinDepthTransitionValue, MIN_VALUE);                                                               \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, DepthTransitionStart, START);                                                                      \
    AZ_EDITOR_MODE_PASS_CVAR(NAMESPACE, DepthTransitionDuration, MIN_VALUE);

AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeDesaturationPass, 0.5f, 5.0f, 10.0f);
AZ_EDITOR_MODE_PASS_CVAR(cl_editorModeDesaturationPass, DesaturationAmount, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(cl_editorModeDesaturationPass, GrayscaleAmount, 0.5f);

namespace AZ
{
    namespace Render
    {
        /**
         *  The color grading pass.
         */
        class EditorModeDesaturationPass
            : public AZ::RPI::FullscreenTrianglePass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeDesaturationPass, "{3587B748-7EA8-497F-B2D1-F60E369EACF4}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(EditorModeDesaturationPass, SystemAllocator, 0);

            virtual ~EditorModeDesaturationPass() = default;

            //! Creates a EditorModeDesaturationPass
            static RPI::Ptr<EditorModeDesaturationPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            EditorModeDesaturationPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_minDepthTransitionValueIndex = "m_minDepthTransitionValue";
            RHI::ShaderInputNameIndex m_depthTransitionStartIndex = "m_depthTransitionStart";
            RHI::ShaderInputNameIndex m_depthTransitionDurationIndex = "m_depthTransitionDuration";

            RHI::ShaderInputNameIndex m_desaturationAmountIndex = "m_desaturationAmount";
            RHI::ShaderInputNameIndex m_grayscaleAmountIndex = "m_grayscaleAmount";
        };
    }   // namespace Render
}   // namespace AZ
