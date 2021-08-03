
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <Editor/EditorSubComponentModeBase.h>
#include <Editor/EditorViewportEntityPicker.h>

namespace AzToolsFramework
{
    class LinearManipulator;
}

namespace PhysX
{
    /// This sub-component mode uses EditorViewportEntityPicker to get the position of an entity that the mouse is hovering over.
    /// Classes inheriting from this class can use the mouse-over entity position to perform custom actions.
    class EditorSubComponentModeSnap
        : public PhysX::EditorSubComponentModeBase
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        EditorSubComponentModeSnap(
            const AZ::EntityComponentIdPair& entityComponentIdPair
            , const AZ::Uuid& componentType
            , const AZStd::string& name);
        virtual ~EditorSubComponentModeSnap() = default;

        // PhysX::EditorSubComponentModeBase
        void HandleMouseInteraction(
            const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        void Refresh() override;

    protected:
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        /// Override to draw specific snap type display
        virtual void DisplaySpecificSnapType(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay,
            [[maybe_unused]] const AZ::Vector3& jointPosition,
            [[maybe_unused]] const AZ::Vector3& snapDirection,
            [[maybe_unused]] float snapLength) {}

        virtual void InitMouseDownCallBack() = 0;

        AZStd::string GetPickedEntityName();
        AZ::Vector3 GetPosition() const;

        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_manipulator;

        EditorViewportEntityPicker m_picker;
        AZ::EntityId m_pickedEntity;
        AZ::Aabb m_pickedEntityAabb = AZ::Aabb::CreateNull();
        AZ::Vector3 m_pickedPosition;
    };
} // namespace PhysX
