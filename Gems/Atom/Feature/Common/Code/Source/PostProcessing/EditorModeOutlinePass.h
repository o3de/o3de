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
        class EditorModeOutlinePass
            : public AZ::RPI::FullscreenTrianglePass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeOutlinePass, "{5DEBA4FC-6BB3-417B-B052-7CB87EF15F84}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(EditorModeOutlinePass, SystemAllocator, 0);

            virtual ~EditorModeOutlinePass() = default;

            //! Creates a EditorModeOutlinePass
            static RPI::Ptr<EditorModeOutlinePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            EditorModeOutlinePass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;
        };
    }   // namespace Render
}   // namespace AZ
