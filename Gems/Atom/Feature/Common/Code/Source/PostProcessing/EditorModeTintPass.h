/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PostProcessing/EditorModeFeedbackPassBase.h>

namespace AZ
{
    namespace Render
    {
        /**
         *  The color grading pass.
         */
        class EditorModeTintPass
            : public EditorModeFeedbackPassBase
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeTintPass, "{3E4FEFCB-9416-4CAE-8918-72D31AA482C5}", EditorModeFeedbackPassBase);
            AZ_CLASS_ALLOCATOR(EditorModeTintPass, SystemAllocator, 0);

            virtual ~EditorModeTintPass() = default;

            //! Creates a EditorModeTintPass
            static RPI::Ptr<EditorModeTintPass> Create(const RPI::PassDescriptor& descriptor);

            void SetTintAmount(float value);
            void SetTintColor(AZ::Color color);

        protected:
            EditorModeTintPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_tintAmountIndex = "m_tintAmount";
            RHI::ShaderInputNameIndex m_tintColorIndex = "m_tintColor";

            float m_tintAmount = 0.25f;
            AZ::Color m_tintColor = AZ::Color::CreateZero();
        };
    }   // namespace Render
}   // namespace AZ
