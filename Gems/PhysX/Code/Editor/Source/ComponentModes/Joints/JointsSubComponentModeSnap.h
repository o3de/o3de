/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <Editor/EditorViewportEntityPicker.h>
#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>

namespace AzToolsFramework
{
    class LinearManipulator;
}

namespace PhysX
{
    class JointsSubComponentModeSnap
        : public PhysXSubComponentModeBase
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        JointsSubComponentModeSnap() = default;

        // PhysXSubComponentModeBase ...
        virtual void Setup(const AZ::EntityComponentIdPair& idPair) override;
        virtual void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        virtual void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;

    protected:
        AZStd::string GetPickedEntityName();
        AZ::Vector3 GetPosition() const;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        //! Override to draw specific snap type display
        virtual void DisplaySpecificSnapType(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay,
            [[maybe_unused]] const AZ::Vector3& jointPosition,
            [[maybe_unused]] const AZ::Vector3& snapDirection,
            [[maybe_unused]] float snapLength)
        {
        }

        EditorViewportEntityPicker m_picker;
        AZ::EntityId m_pickedEntity;
        AZ::Aabb m_pickedEntityAabb = AZ::Aabb::CreateNull();
        AZ::Vector3 m_pickedPosition;
        AZ::EntityComponentIdPair m_entityComponentId;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_manipulator;
    };
}
