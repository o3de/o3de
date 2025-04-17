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
        //! Pass for editor mode feedback desaturation effect.
        class EditorModeDesaturationPass
            : public EditorModeFeedbackChildPassBase
        {
        public:
            AZ_RTTI(EditorModeDesaturationPass, "{3587B748-7EA8-497F-B2D1-F60E369EACF4}", EditorModeFeedbackChildPassBase);
            AZ_CLASS_ALLOCATOR(EditorModeDesaturationPass, SystemAllocator);

            virtual ~EditorModeDesaturationPass() = default;

            //! Creates a EditorModeDesaturationPass
            static RPI::Ptr<EditorModeDesaturationPass> Create(const RPI::PassDescriptor& descriptor);

            //! Sets the amount of desaturation to apply.
            void SetDesaturationAmount(float amount);

        protected:
            EditorModeDesaturationPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            //! Sets the shader constant values for the desaturation effect.
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_desaturationAmountIndex = "m_desaturationAmount";
            float m_desaturationAmount = 1.0f; //!< Default desaturation amount for the desaturation effect.
        };
    }   // namespace Render
}   // namespace AZ
