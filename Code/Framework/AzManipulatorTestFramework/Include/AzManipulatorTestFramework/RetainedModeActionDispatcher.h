/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/ActionDispatcher.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/functional.h>

namespace AzManipulatorTestFramework
{
    //! Buffers actions to be dispatched upon a call to Execute().
    class RetainedModeActionDispatcher
        : public ActionDispatcher<RetainedModeActionDispatcher>
    {
    public:
        explicit RetainedModeActionDispatcher(ManipulatorViewportInteraction& viewportManipulatorInteraction);
        //! Execute the sequence of actions and lock the dispatcher from adding further actions.
        RetainedModeActionDispatcher* Execute();
        //! Reset the sequence of actions and unlock the dispatcher from adding further actions.
        RetainedModeActionDispatcher* ResetSequence();

    protected:
        // ActionDispatcher ...
        void EnableSnapToGridImpl() override;
        void DisableSnapToGridImpl() override;
        void GridSizeImpl(float size) override;
        void CameraStateImpl(const AzFramework::CameraState& cameraState) override;
        void MouseLButtonDownImpl() override;
        void MouseLButtonUpImpl() override;
        void MousePositionImpl(const AzFramework::ScreenPoint& position) override;
        void KeyboardModifierDownImpl(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier) override;
        void KeyboardModifierUpImpl(const AzToolsFramework::ViewportInteraction::KeyboardModifier& keyModifier) override;
        void ExpectManipulatorBeingInteractedImpl() override;
        void ExpectManipulatorNotBeingInteractedImpl() override;
        void SetEntityWorldTransformImpl(AZ::EntityId entityId, const AZ::Transform& transform) override;
        void SetSelectedEntityImpl(AZ::EntityId entity) override;
        void SetSelectedEntitiesImpl(const AzToolsFramework::EntityIdList& entities) override;
        void EnterComponentModeImpl(const AZ::Uuid& uuid) override;

    private:
        using Action = AZStd::function<void()>;
        void AddActionToSequence(Action&& action);
        ImmediateModeActionDispatcher m_dispatcher;
        AZStd::list<Action> m_actions;
        bool m_locked = false;
    };
} // namespace AzManipulatorTestFramework
