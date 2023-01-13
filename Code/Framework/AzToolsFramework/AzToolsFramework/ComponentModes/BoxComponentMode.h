/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzToolsFramework/ComponentModes/ShapeTranslationOffsetViewportEdit.h>

namespace AzToolsFramework
{
    class LinearManipulator;

    /// The specific ComponentMode responsible for handling box editing.
    class BoxComponentMode
        : public ComponentModeFramework::EditorBaseComponentMode
    {
    public:
        enum class SubMode : AZ::u32
        {
            Dimensions,
            TranslationOffset,
            NumModes
        };

        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(BoxComponentMode, "{8E09B2C1-ED99-4945-A0B1-C4AFE6FE2FA9}", EditorBaseComponentMode)

        static void Reflect(AZ::ReflectContext* context);

        BoxComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing = false);
        BoxComponentMode(const BoxComponentMode&) = delete;
        BoxComponentMode& operator=(const BoxComponentMode&) = delete;
        BoxComponentMode(BoxComponentMode&&) = delete;
        BoxComponentMode& operator=(BoxComponentMode&&) = delete;
        ~BoxComponentMode();

        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;
        bool HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

        constexpr static const char* const DimensionsTooltip = "Switch to dimensions mode";
        constexpr static const char* const TranslationOffsetTooltip = "Switch to translation offset mode";

    private:
        void SetupCluster();
        void SetCurrentMode(SubMode mode);

        ViewportUi::ClusterId m_clusterId; //! Id for viewport cluster used to switch between modes.
        ViewportUi::ButtonId m_dimensionsButtonId; 
        ViewportUi::ButtonId m_translationOffsetButtonId;
        AZStd::array<ViewportUi::ButtonId, 2> m_buttonIds;
        AZStd::array<AZStd::unique_ptr<BaseViewportEdit>, 2> m_subModes;
        SubMode m_subMode = SubMode::Dimensions;
        bool m_allowAsymmetricalEditing = false;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_modeSelectionHandler;
        AZ::EntityComponentIdPair m_entityComponentIdPair;
    };
} // namespace AzToolsFramework
