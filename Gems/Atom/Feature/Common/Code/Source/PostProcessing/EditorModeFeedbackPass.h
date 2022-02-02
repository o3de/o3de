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
        class EditorModeFeedbackPass
            : public AZ::RPI::FullscreenTrianglePass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeFeedbackPass, "{3587B748-7EA8-497F-B2D1-F60E369EACF4}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(EditorModeFeedbackPass, SystemAllocator, 0);

            virtual ~EditorModeFeedbackPass() = default;

            //! Creates a EditorModeFeedbackPass
            static RPI::Ptr<EditorModeFeedbackPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            EditorModeFeedbackPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_desaturationAmountIndex = "m_desaturationAmount";
        };
    }   // namespace Render
}   // namespace AZ
