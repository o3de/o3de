/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentModes/BaseShapeViewportEdit.h>
#include <AzToolsFramework/ComponentModes/ShapeComponentModeBus.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>

namespace AzToolsFramework
{
    void InstallBaseShapeViewportEditFunctions(
        BaseShapeViewportEdit* baseShapeViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair);

    //! Base class for shape component modes.
    //! Handles common logic such as setting up sub-modes for dimensions and translation offset, handling mode
    //! selection, registering actions, etc.
    class BaseShapeComponentMode
        : public ComponentModeFramework::EditorBaseComponentMode
        , public ShapeComponentModeRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(BaseShapeComponentMode, "{19DE4C0C-4918-4D1B-8F3D-365D731AC20C}", EditorBaseComponentMode)

        static void RegisterActions(const char* shapeName);
        static void BindActionsToModes(const char* shapeName, const char* className);
        static void BindActionsToMenus(const char* shapeName);

        BaseShapeComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType, bool allowAsymmetricalEditing = false);
        BaseShapeComponentMode(const BaseShapeComponentMode&) = delete;
        BaseShapeComponentMode& operator=(const BaseShapeComponentMode&) = delete;
        BaseShapeComponentMode(BaseShapeComponentMode&&) = delete;
        BaseShapeComponentMode& operator=(BaseShapeComponentMode&&) = delete;
        virtual ~BaseShapeComponentMode();

        // EditorBaseComponentMode overrides ...
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

        // ShapeComponentModeRequestBus overrides ...
        ShapeComponentModeRequests::SubMode GetShapeSubMode() const override;
        void SetShapeSubMode(ShapeComponentModeRequests::SubMode mode) override;
        void ResetShapeSubMode() override;

        constexpr static const char* const DimensionsTooltip = "Switch to dimensions mode (1)";
        constexpr static const char* const TranslationOffsetTooltip = "Switch to translation offset mode (2)";

    protected:
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
