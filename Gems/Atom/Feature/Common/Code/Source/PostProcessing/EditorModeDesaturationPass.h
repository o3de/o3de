/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PostProcessing/EditorModeFeedbackDepthTransition.h>

#include <AzCore/Console/IConsole.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>

AZ_EDITOR_MODE_PASS_TRANSITION_CVARS(cl_editorModeDesaturationPass, 0.5f, 5.0f, 10.0f, 1.0f);
AZ_EDITOR_MODE_PASS_CVAR(cl_editorModeDesaturationPass, DesaturationAmount, 1.0f);

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

            EditorModeFeedbackDepthTransition m_depthTransition;
            RHI::ShaderInputNameIndex m_desaturationAmountIndex = "m_desaturationAmount";
        };
    }   // namespace Render
}   // namespace AZ
