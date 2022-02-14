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
        class EditorModeOutlinePass
            : public EditorModeFeedbackPassBase
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(EditorModeOutlinePass, "{5DEBA4FC-6BB3-417B-B052-7CB87EF15F84}", EditorModeFeedbackPassBase);
            AZ_CLASS_ALLOCATOR(EditorModeOutlinePass, SystemAllocator, 0);

            virtual ~EditorModeOutlinePass() = default;

            //! Creates a EditorModeOutlinePass
            static RPI::Ptr<EditorModeOutlinePass> Create(const RPI::PassDescriptor& descriptor);

            void SetLineThickness(float width);
            void SetLineColor(AZ::Color color);

        protected:
            EditorModeOutlinePass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

        private:
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_lineThicknessIndex = "m_lineThickness";
            RHI::ShaderInputNameIndex m_lineColorIndex = "m_lineColor";
            float m_lineThickness = 3.0f;
            AZ::Color m_lineColor = AZ::Color(0.96f, 0.65f, 0.13f, 1.0f);
        };
    }   // namespace Render
}   // namespace AZ
