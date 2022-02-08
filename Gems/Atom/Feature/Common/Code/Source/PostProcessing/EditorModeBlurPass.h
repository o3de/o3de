/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <PostProcess/EditorModeFeedback/EditorModeFeedbackSettings.h>

namespace AZ
{
    namespace Render
    {
        /**
         *  The color grading pass.
         */
        class EditorModeBlurPass
            : public AZ::RPI::FullscreenTrianglePass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeBlurPass, "{D907D0ED-61E4-4E46-A682-A849676CF48A}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(EditorModeBlurPass, SystemAllocator, 0);

            virtual ~EditorModeBlurPass() = default;

            //! Creates a EditorModeBlurPass
            static RPI::Ptr<EditorModeBlurPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            EditorModeBlurPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;
        };
    }   // namespace Render
}   // namespace AZ
