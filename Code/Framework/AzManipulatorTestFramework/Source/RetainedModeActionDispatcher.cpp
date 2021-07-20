/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzManipulatorTestFramework/RetainedModeActionDispatcher.h>

namespace AzManipulatorTestFramework
{
    using KeyboardModifier = AzToolsFramework::ViewportInteraction::KeyboardModifier;

    RetainedModeActionDispatcher::RetainedModeActionDispatcher(
        ManipulatorViewportInteraction& viewportManipulatorInteraction)
        : m_dispatcher(viewportManipulatorInteraction)
    {
    }

    void RetainedModeActionDispatcher::AddActionToSequence(Action&& action)
    {
        if (m_locked)
        {
            const char* error = "Couldn't add action to sequence, dispatcher is locked (you must call ResetSequence() \
                                 before adding actions to this dispatcher)";
            Log(error);
            AZ_Assert(false, "Error: %s", error);
        }

        m_actions.emplace_back(action);
    }

    void RetainedModeActionDispatcher::EnableSnapToGridImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.EnableSnapToGrid(); });
    }

    void RetainedModeActionDispatcher::DisableSnapToGridImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.DisableSnapToGrid(); });
    }

    void RetainedModeActionDispatcher::GridSizeImpl(float size)
    {
        AddActionToSequence([=]() { m_dispatcher.GridSize(size); });
    }

    void RetainedModeActionDispatcher::CameraStateImpl(const AzFramework::CameraState& cameraState)
    {
        AddActionToSequence([=]() { m_dispatcher.CameraState(cameraState); });
    }

    void RetainedModeActionDispatcher::MouseLButtonDownImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.MouseLButtonDown(); });
    }

    void RetainedModeActionDispatcher::MouseLButtonUpImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.MouseLButtonUp(); });
    }

    void RetainedModeActionDispatcher::MousePositionImpl(const AzFramework::ScreenPoint& position)
    {
        AddActionToSequence([=]() { m_dispatcher.MousePosition(position); });
    }

    void RetainedModeActionDispatcher::KeyboardModifierDownImpl(const KeyboardModifier& keyModifier)
    {
        AddActionToSequence([=]() { m_dispatcher.KeyboardModifierDown(keyModifier); });
    }

    void RetainedModeActionDispatcher::KeyboardModifierUpImpl(const KeyboardModifier& keyModifier)
    {
        AddActionToSequence([=]() { m_dispatcher.KeyboardModifierUp(keyModifier); });
    }

    void RetainedModeActionDispatcher::ExpectManipulatorBeingInteractedImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.ExpectManipulatorBeingInteracted(); });
    }

    void RetainedModeActionDispatcher::ExpectManipulatorNotBeingInteractedImpl()
    {
        AddActionToSequence([=]() { m_dispatcher.ExpectManipulatorNotBeingInteracted(); });
    }

    void RetainedModeActionDispatcher::SetEntityWorldTransformImpl(AZ::EntityId entityId, const AZ::Transform& transform)
    {
        AddActionToSequence([=]() { m_dispatcher.SetEntityWorldTransform(entityId, transform); });
    }

    void RetainedModeActionDispatcher::SetSelectedEntityImpl(AZ::EntityId entity)
    {
        AddActionToSequence([=]() { m_dispatcher.SetSelectedEntity(entity); });
    }

    void RetainedModeActionDispatcher::SetSelectedEntitiesImpl(const AzToolsFramework::EntityIdList& entities)
    {
        AddActionToSequence([=]() { m_dispatcher.SetSelectedEntities(entities); });
    }

    void RetainedModeActionDispatcher::EnterComponentModeImpl(const AZ::Uuid& uuid)
    {
        AddActionToSequence([=]() { m_dispatcher.EnterComponentMode(uuid); });
    }

    RetainedModeActionDispatcher* RetainedModeActionDispatcher::ResetSequence()
    {
        Log("Resetting the action sequence");
        m_actions.clear();
        m_dispatcher.ResetEvent();
        m_locked = false;
        return this;
    }

    RetainedModeActionDispatcher* RetainedModeActionDispatcher::Execute()
    {
        Log("Executing %u actions", m_actions.size());
        for (auto& action : m_actions)
        {
            action();
        }
        m_dispatcher.ResetEvent();
        m_locked = true;
        return this;
    }
} // namespace AzManipulatorTestFramework
