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
#include <AzToolsFramework/ComponentModes/ShapeComponentModeBus.h>
#include <AzToolsFramework/ComponentModes/ShapeTranslationOffsetViewportEdit.h>

namespace AzToolsFramework
{
    class LinearManipulator;

    void InstallBaseShapeViewportEditFunctions(
        BaseShapeViewportEdit* baseShapeViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair);

    void InstallBoxViewportEditFunctions(BoxViewportEdit* boxViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair);

    //! The specific ComponentMode responsible for handling box editing.
    class BoxComponentMode
        : public ComponentModeFramework::EditorBaseComponentMode
        , public ShapeComponentModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(BoxComponentMode, "{8E09B2C1-ED99-4945-A0B1-C4AFE6FE2FA9}", EditorBaseComponentMode)

        static void Reflect(AZ::ReflectContext* context);

        static void RegisterActions();
        static void BindActionsToModes();
        static void BindActionsToMenus();

        BoxComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType, bool allowAsymmetricalEditing = false);
        BoxComponentMode(const BoxComponentMode&) = delete;
        BoxComponentMode& operator=(const BoxComponentMode&) = delete;
        BoxComponentMode(BoxComponentMode&&) = delete;
        BoxComponentMode& operator=(BoxComponentMode&&) = delete;
        ~BoxComponentMode();

        // EditorBaseComponentMode overrides ...
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        AZStd::string GetComponentModeName() const override;
        AZ::Uuid GetComponentModeType() const override;
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

        // ShapeComponentModeRequestBus overrides ...
        ShapeComponentModeRequests::SubMode GetShapeSubMode() const override;
        void SetShapeSubMode(ShapeComponentModeRequests::SubMode mode) override;
        void ResetShapeSubMode() override;

        constexpr static const char* const DimensionsTooltip = "Switch to dimensions mode";
        constexpr static const char* const TranslationOffsetTooltip = "Switch to translation offset mode";

    private:
        void SetupCluster();

        ViewportUi::ClusterId m_clusterId; //! Id for viewport cluster used to switch between modes.
        AZStd::array<ViewportUi::ButtonId, 2> m_buttonIds;
        AZStd::array<AZStd::unique_ptr<BaseShapeViewportEdit>, 2> m_subModes;
        ShapeComponentModeRequests::SubMode m_subMode = ShapeComponentModeRequests::SubMode::Dimensions;
        bool m_allowAsymmetricalEditing = false;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_modeSelectionHandler;
        AZ::EntityComponentIdPair m_entityComponentIdPair;
    };
} // namespace AzToolsFramework
