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
        class EditorModeBlurPass
            : public EditorModeFeedbackPassBase
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeBlurPass, "{D907D0ED-61E4-4E46-A682-A849676CF48A}", EditorModeFeedbackPassBase);
            AZ_CLASS_ALLOCATOR(EditorModeBlurPass, SystemAllocator, 0);

            virtual ~EditorModeBlurPass() = default;

            //! Creates a EditorModeBlurPass
            static RPI::Ptr<EditorModeBlurPass> Create(const RPI::PassDescriptor& descriptor);

            void SetKernalWidth(float width);

        protected:
            EditorModeBlurPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_kernalWidthIndex = "m_kernalWidth";
            float m_kernalWidth = 1.0f;
        };
    }   // namespace Render
}   // namespace AZ
