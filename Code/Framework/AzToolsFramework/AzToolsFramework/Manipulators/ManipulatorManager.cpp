/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ManipulatorManager.h"
#include "BaseManipulator.h"

#include <AzCore/std/tuple.h>
#include <AzToolsFramework/Picking/BoundInterface.h>

namespace AzToolsFramework
{
    const ManipulatorManagerId g_mainManipulatorManagerId = ManipulatorManagerId(AZ::Crc32("MainManipulatorManagerId"));

    ManipulatorManager::ManipulatorManager(const ManipulatorManagerId managerId)
        : m_manipulatorManagerId(managerId)
        , m_nextManipulatorIdToGenerate(ManipulatorId(1))
    {
        ManipulatorManagerRequestBus::Handler::BusConnect(m_manipulatorManagerId);
        EditorEntityInfoNotificationBus::Handler::BusConnect();
    }

    ManipulatorManager::~ManipulatorManager()
    {
        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            pair.second->Invalidate();
        }

        m_manipulatorIdToPtrMap.clear();

        ManipulatorManagerRequestBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
    }

    void ManipulatorManager::RegisterManipulator(AZStd::shared_ptr<BaseManipulator> manipulator)
    {
        if (!manipulator)
        {
            AZ_Error("Manipulators", false, "Attempting to register a null Manipulator");
            return;
        }

        if (manipulator->Registered())
        {
            AZ_Assert(
                manipulator->GetManipulatorManagerId() == m_manipulatorManagerId,
                "This manipulator was registered with a different manipulator manager!");
            return;
        }

        const ManipulatorId manipulatorId = m_nextManipulatorIdToGenerate++;
        manipulator->m_manipulatorId = manipulatorId;
        manipulator->m_manipulatorManagerId = m_manipulatorManagerId;
        m_manipulatorIdToPtrMap[manipulatorId] = AZStd::move(manipulator);
    }

    void ManipulatorManager::UnregisterManipulator(BaseManipulator* manipulator)
    {
        if (!manipulator)
        {
            AZ_Error("Manipulators", false, "Attempting to unregister a null Manipulator");
            return;
        }

        m_manipulatorIdToPtrMap.erase(manipulator->GetManipulatorId());
        manipulator->Invalidate();
    }

    Picking::RegisteredBoundId ManipulatorManager::UpdateBound(
        const ManipulatorId manipulatorId, const Picking::RegisteredBoundId boundId, const Picking::BoundRequestShapeBase& boundShapeData)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (manipulatorId == InvalidManipulatorId)
        {
            return Picking::InvalidBoundId;
        }

        const auto manipulatorItr = m_manipulatorIdToPtrMap.find(manipulatorId);
        if (manipulatorItr == m_manipulatorIdToPtrMap.end())
        {
            return Picking::InvalidBoundId;
        }

        if (boundId != Picking::InvalidBoundId)
        {
            auto boundItr = m_boundIdToManipulatorIdMap.find(boundId);
            AZ_UNUSED(boundItr);
            AZ_Assert(boundItr != m_boundIdToManipulatorIdMap.end(), "Manipulator and its bounds are out of synchronization!");
            AZ_Assert(boundItr->second == manipulatorId, "Manipulator and its bounds are out of synchronization!");
        }

        const Picking::RegisteredBoundId newBoundId = m_boundManager.UpdateOrRegisterBound(boundShapeData, boundId);

        if (newBoundId != boundId)
        {
            m_boundIdToManipulatorIdMap[newBoundId] = manipulatorId;
        }

        return newBoundId;
    }

    void ManipulatorManager::SetBoundDirty(const Picking::RegisteredBoundId boundId)
    {
        if (boundId != Picking::InvalidBoundId)
        {
            m_boundManager.SetBoundValidity(boundId, false);
        }
    }

    void ManipulatorManager::DeleteManipulatorBound(const Picking::RegisteredBoundId boundId)
    {
        if (boundId != Picking::InvalidBoundId)
        {
            m_boundManager.UnregisterBound(boundId);
            m_boundIdToManipulatorIdMap.erase(boundId);
        }
    }

    void ManipulatorManager::RefreshMouseOverState(const ViewportInteraction::MousePick& mousePick)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!Interacting())
        {
            auto [pickedManipulatorId, _] = PickManipulatorId(mousePick);

            for (auto& pair : m_manipulatorIdToPtrMap)
            {
                pair.second->UpdateMouseOver(pickedManipulatorId);
            }
        }
    }

    void ManipulatorManager::DrawManipulators(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        for (const auto& pair : m_manipulatorIdToPtrMap)
        {
            pair.second->Draw(ManipulatorManagerState{ Interacting() }, debugDisplay, cameraState, mouseInteraction);
        }

        RefreshMouseOverState(mouseInteraction.m_mousePick);
    }

    AZStd::shared_ptr<BaseManipulator> ManipulatorManager::PerformRaycast(
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, float& rayIntersectionDistance)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        Picking::RaySelectInfo raySelection;
        raySelection.m_origin = rayOrigin;
        raySelection.m_direction = rayDirection;

        m_boundManager.RaySelect(raySelection);

        for (const auto& hitItr : raySelection.m_boundIdsHit)
        {
            const auto found = m_boundIdToManipulatorIdMap.find(hitItr.first);
            if (found != m_boundIdToManipulatorIdMap.end())
            {
                const auto manipulatorFound = m_manipulatorIdToPtrMap.find(found->second);
                AZ_Assert(
                    manipulatorFound != m_manipulatorIdToPtrMap.end(),
                    "Found a bound without a corresponding Manipulator, "
                    "it's likely a bound was not cleaned up correctly");
                rayIntersectionDistance = hitItr.second;
                return manipulatorFound != m_manipulatorIdToPtrMap.end() ? manipulatorFound->second : nullptr;
            }
        }

        return nullptr;
    }

    bool ManipulatorManager::ConsumeViewportMousePress(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (auto pickedManipulator = PickManipulator(interaction.m_mousePick); pickedManipulator.has_value())
        {
            auto [manipulator, intersectionDistance] = pickedManipulator.value();

            if (interaction.m_mouseButtons.Left())
            {
                if (manipulator->OnLeftMouseDown(interaction, intersectionDistance))
                {
                    m_activeManipulator = manipulator;
                    return true;
                }
            }

            if (interaction.m_mouseButtons.Right())
            {
                if (manipulator->OnRightMouseDown(interaction, intersectionDistance))
                {
                    m_activeManipulator = manipulator;
                    return true;
                }
            }
        }

        return false;
    }

    bool ManipulatorManager::ConsumeViewportMouseRelease(const ViewportInteraction::MouseInteraction& interaction)
    {
        // must have had a meaningful interaction in mouse down to have assigned an
        // active manipulator - only notify mouse up if this was the case
        if (m_activeManipulator)
        {
            if (interaction.m_mouseButtons.Left())
            {
                m_activeManipulator->OnLeftMouseUp(interaction);
                m_activeManipulator = nullptr;
                return true;
            }

            if (interaction.m_mouseButtons.Right())
            {
                m_activeManipulator->OnRightMouseUp(interaction);
                m_activeManipulator = nullptr;
                return true;
            }
        }

        return false;
    }

    AZStd::optional<ManipulatorManager::PickedManipulator> ManipulatorManager::PickManipulator(
        const ViewportInteraction::MousePick& mousePick)
    {
        float intersectionDistance = 0.0f;
        const AZStd::shared_ptr<BaseManipulator> pickedManipulator =
            PerformRaycast(mousePick.m_rayOrigin, mousePick.m_rayDirection, intersectionDistance);

        return pickedManipulator.get() != nullptr ? AZStd::make_optional(AZStd::make_tuple(pickedManipulator, intersectionDistance))
                                                  : AZStd::nullopt;
    }

    ManipulatorManager::PickedManipulatorId ManipulatorManager::PickManipulatorId(const ViewportInteraction::MousePick& mousePick)
    {
        auto [manipulator, intersectionDistance] = PickManipulator(mousePick).value_or(PickedManipulator(nullptr, 0.0f));
        const ManipulatorId pickedManipulatorId = manipulator ? manipulator->GetManipulatorId() : InvalidManipulatorId;

        return PickedManipulatorId{ pickedManipulatorId, intersectionDistance };
    }

    ManipulatorManager::ConsumeMouseMoveResult ManipulatorManager::ConsumeViewportMouseMove(
        const ViewportInteraction::MouseInteraction& interaction)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_activeManipulator)
        {
            m_activeManipulator->OnMouseMove(interaction);
            return ConsumeMouseMoveResult::Interacting;
        }

        return ConsumeMouseMoveResult::None;
    }

    bool ManipulatorManager::ConsumeViewportMouseWheel(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_activeManipulator)
        {
            m_activeManipulator->OnMouseWheel(interaction);
            return true;
        }

        return false;
    }

    void ManipulatorManager::OnEntityInfoUpdatedVisibility(const AZ::EntityId entityId, const bool visible)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        for (auto& pair : m_manipulatorIdToPtrMap)
        {
            // set all manipulator bounds on this entity to dirty so we cannot
            // interact with them (bounds will be refreshed when they are redrawn)
            for (const AZ::EntityComponentIdPair& id : pair.second->EntityComponentIdPairs())
            {
                if (id.GetEntityId() == entityId && !visible)
                {
                    pair.second->SetBoundsDirty();
                    break;
                }
            }
        }
    }
} // namespace AzToolsFramework
