/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include "ColliderComponentModeBus.h"

namespace PhysX
{
    class PhysXSubComponentModeBase;

    //! ComponentMode for the Collider Component - Manages a list of Sub-Component Modes and 
    //! is responsible for switching between and activating them.
    class ColliderComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , public ColliderComponentModeRequestBus::Handler
        , public ColliderComponentModeUiRequestBus::Handler
    {
    public:

        AZ_CLASS_ALLOCATOR_DECL;

        ColliderComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~ColliderComponentMode();

        // EditorBaseComponentMode overrides ...
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        AZStd::vector<AzToolsFramework::ViewportUi::ClusterId> PopulateViewportUiImpl() override;

        // ColliderComponentModeBus overrides ...
        SubMode GetCurrentMode() override;
        void SetCurrentMode(SubMode index) override;

        // ColliderComponentModeUiBus overrides ...
        AzToolsFramework::ViewportUi::ButtonId GetOffsetButtonId() const override;
        AzToolsFramework::ViewportUi::ButtonId GetRotationButtonId() const override;
        AzToolsFramework::ViewportUi::ClusterId GetClusterId() const override;
        AzToolsFramework::ViewportUi::ButtonId GetDimensionsButtonId() const override;

        // ComponentMode overrides ...
        AZStd::string GetComponentModeName() const override;
    private:
        
        // AzToolsFramework::ViewportInteraction::ViewportSelectionRequests ...
        bool HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        void CreateSubModes();
        void ResetCurrentMode();

        AZStd::unordered_map<SubMode, AZStd::unique_ptr<PhysXSubComponentModeBase>> m_subModes;
        SubMode m_subMode = SubMode::Dimensions;

        //! Create the Viewport UI cluster for sub mode selection.
        void CreateSubModeSelectionCluster();
        //! Remove the Viewport UI cluster for sub mode selection.
        void RemoveSubModeSelectionCluster();

        AzToolsFramework::ViewportUi::ClusterId
            m_modeSelectionClusterId; //!< Viewport UI cluster for changing sub mode.

        AZStd::vector<AzToolsFramework::ViewportUi::ButtonId> m_buttonIds; //!< Ids for the Viewport UI buttons for each mode.

        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler
            m_modeSelectionHandler; //!< Event handler for sub mode changes.
    };
} // namespace PhysX
