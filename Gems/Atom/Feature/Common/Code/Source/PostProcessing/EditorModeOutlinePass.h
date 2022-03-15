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
        //! Pass for editor mode feedback outline effect.
        class EditorModeOutlinePass
            : public EditorModeFeedbackPassBase
        {
        public:
            enum class LineMode : AZ::u32
            {
                OutlineAlways,
                OutlineVisible
            };

            AZ_RTTI(EditorModeOutlinePass, "{5DEBA4FC-6BB3-417B-B052-7CB87EF15F84}", EditorModeFeedbackPassBase);
            AZ_CLASS_ALLOCATOR(EditorModeOutlinePass, SystemAllocator, 0);

            virtual ~EditorModeOutlinePass() = default;

            //! Creates a EditorModeOutlinePass
            static RPI::Ptr<EditorModeOutlinePass> Create(const RPI::PassDescriptor& descriptor);

            //! Sets the outline line thickness.
            void SetLineThickness(float thickness);

            //! Sets the outline line color.
            void SetLineColor(AZ::Color color);

            //! Sets the outline mode.
            void SetLineMode(LineMode mode);

        protected:
            EditorModeOutlinePass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            //! Sets the shader constant values for the outline effect.
            void SetSrgConstants();

            RHI::ShaderInputNameIndex m_lineThicknessIndex = "m_lineThickness";
            RHI::ShaderInputNameIndex m_lineColorIndex = "m_lineColor";
            RHI::ShaderInputNameIndex m_lineModeIndex = "m_lineMode";
            float m_lineThickness = 3.0f; //!< Default line thickness for the outline effect.
            AZ::Color m_lineColor = AZ::Color(0.96f, 0.65f, 0.13f, 1.0f); //!< Default line color for the outline effect.
            LineMode m_lineMode = LineMode::OutlineAlways;
        };
    }   // namespace Render
}   // namespace AZ
