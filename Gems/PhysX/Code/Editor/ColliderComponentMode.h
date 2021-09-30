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

        // EditorBaseComponentMode ...
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;
        AZStd::vector<AzToolsFramework::ViewportUi::ClusterId> PopulateViewportUiImpl() override;

        // ColliderComponentModeBus ...
        SubMode GetCurrentMode() override;
        void SetCurrentMode(SubMode index) override;

        // ColliderComponentModeUiBus ...
        AzToolsFramework::ViewportUi::ButtonId GetOffsetButtonId() const override;
        AzToolsFramework::ViewportUi::ButtonId GetRotationButtonId() const override;
        AzToolsFramework::ViewportUi::ClusterId GetClusterId() const override;
        AzToolsFramework::ViewportUi::ButtonId GetDimensionsButtonId() const override;

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
        AzToolsFramework::ViewportUi::ButtonId
            m_dimensionsModeButtonId; //!< Id of the Viewport UI button for resize/dimensions mode.
        AzToolsFramework::ViewportUi::ButtonId
            m_offsetModeButtonId; //!< Id of the Viewport UI button for offset mode.
        AzToolsFramework::ViewportUi::ButtonId
            m_rotationModeButtonId; //!< Id of the Viewport UI button for rotation mode.
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler
            m_modeSelectionHandler; //!< Event handler for sub mode changes.
    };
} // namespace PhysX
