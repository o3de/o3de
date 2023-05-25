/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Pass/Child/EditorModeFeedbackChildPassBase.h>

namespace AZ
{
    namespace Render
    {
        //! Pass for editor mode feedback color tint effect.
        class EditorModeTintPass
            : public EditorModeFeedbackChildPassBase
        {
        public:
            AZ_RTTI(EditorModeTintPass, "{3E4FEFCB-9416-4CAE-8918-72D31AA482C5}", EditorModeFeedbackChildPassBase);
            AZ_CLASS_ALLOCATOR(EditorModeTintPass, SystemAllocator);

            virtual ~EditorModeTintPass() = default;

            //! Creates a EditorModeTintPass
            static RPI::Ptr<EditorModeTintPass> Create(const RPI::PassDescriptor& descriptor);

            //! Sets the amount of tint to apply.
            void SetTintAmount(float amount);

            //! Sets the color of tint to apply.
            void SetTintColor(AZ::Color color);

        protected:
            EditorModeTintPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            //! Sets the shader constant values for the color tint effect.
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_tintAmountIndex = "m_tintAmount";
            RHI::ShaderInputNameIndex m_tintColorIndex = "m_tintColor";
            float m_tintAmount = 0.5f; //!< Default tint amount for the color tint effect.
            AZ::Color m_tintColor = AZ::Color::CreateZero(); //!< Default tint color for the color tint effect.
        };
    }   // namespace Render
}   // namespace AZ
