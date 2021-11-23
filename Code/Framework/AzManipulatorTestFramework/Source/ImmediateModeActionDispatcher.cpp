/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzManipulatorTestFramework
{
    template<typename FieldT, typename FlagT>
    void ToggleOn(FieldT& field, FlagT flag)
    {
        field |= static_cast<FieldT>(flag);
    }

    template<typename FieldT, typename FlagT>
    void ToggleOff(FieldT& field, FlagT flag)
    {
        field &= ~static_cast<FieldT>(flag);
    }

    using MouseButton = AzToolsFramework::ViewportInteraction::MouseButton;
    using KeyboardModifier = AzToolsFramework::ViewportInteraction::KeyboardModifier;
    using MouseInteractionEvent = AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    ImmediateModeActionDispatcher::ImmediateModeActionDispatcher(ManipulatorViewportInteraction& viewportManipulatorInteraction)
        : m_viewportManipulatorInteraction(viewportManipulatorInteraction)
    {
        AzToolsFramework::ViewportInteraction::EditorModifierKeyRequestBus::Handler::BusConnect();
        AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequestBus::Handler::BusConnect();
    }

    ImmediateModeActionDispatcher::~ImmediateModeActionDispatcher()
    {
        AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequestBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportInteraction::EditorModifierKeyRequestBus::Handler::BusDisconnect();
    }

    void ImmediateModeActionDispatcher::MouseMoveAfterButton()
    {
        // the editor application generates a mouse move event with a zero delta after every
        // mouse down and mouse up event, to match the editor behavior we insert this event
        // to ensure the tests are simulating the same environment as the editor
        GetMouseInteractionEvent()->m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Move;
        m_viewportManipulatorInteraction.GetManipulatorManager().ConsumeMouseInteractionEvent(*m_event);
    }

    void ImmediateModeActionDispatcher::SetSnapToGridImpl(const bool enabled)
    {
        m_viewportManipulatorInteraction.GetViewportInteraction().SetGridSnapping(enabled);
    }

    void ImmediateModeActionDispatcher::SetStickySelectImpl(const bool enabled)
    {
        m_viewportManipulatorInteraction.GetViewportInteraction().SetStickySelect(enabled);
    }

    void ImmediateModeActionDispatcher::GridSizeImpl(const float size)
    {
        m_viewportManipulatorInteraction.GetViewportInteraction().SetGridSize(size);
    }

    void ImmediateModeActionDispatcher::CameraStateImpl(const AzFramework::CameraState& cameraState)
    {
        m_viewportManipulatorInteraction.GetViewportInteraction().SetCameraState(cameraState);
    }

    void ImmediateModeActionDispatcher::MouseLButtonDownImpl()
    {
        ToggleOn(GetMouseInteractionEvent()->m_mouseInteraction.m_mouseButtons.m_mouseButtons, MouseButton::Left);
        GetMouseInteractionEvent()->m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Down;
        m_viewportManipulatorInteraction.GetManipulatorManager().ConsumeMouseInteractionEvent(*m_event);
        // the mouse position will be the same as the previous event, thus the delta will be 0
        MouseMoveAfterButton();
    }

    void ImmediateModeActionDispatcher::MouseLButtonUpImpl()
    {
        GetMouseInteractionEvent()->m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Up;
        m_viewportManipulatorInteraction.GetManipulatorManager().ConsumeMouseInteractionEvent(*m_event);
        ToggleOff(GetMouseInteractionEvent()->m_mouseInteraction.m_mouseButtons.m_mouseButtons, MouseButton::Left);
        // the mouse position will be the same as the previous event, thus the delta will be 0
        MouseMoveAfterButton();
    }

    void ImmediateModeActionDispatcher::MouseLButtonDoubleClickImpl()
    {
        GetMouseInteractionEvent()->m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::DoubleClick;
        ToggleOn(GetMouseInteractionEvent()->m_mouseInteraction.m_mouseButtons.m_mouseButtons, MouseButton::Left);
        m_viewportManipulatorInteraction.GetManipulatorManager().ConsumeMouseInteractionEvent(*m_event);
        ToggleOff(GetMouseInteractionEvent()->m_mouseInteraction.m_mouseButtons.m_mouseButtons, MouseButton::Left);
        // the mouse position will be the same as the previous event, thus the delta will be 0
        MouseMoveAfterButton();
    }

    void ImmediateModeActionDispatcher::MousePositionImpl(const AzFramework::ScreenPoint& position)
    {
        const auto cameraState = m_viewportManipulatorInteraction.GetViewportInteraction().GetCameraState();
        GetMouseInteractionEvent()->m_mouseInteraction.m_mousePick = BuildMousePick(position, cameraState);
        GetMouseInteractionEvent()->m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Move;
        m_viewportManipulatorInteraction.GetManipulatorManager().ConsumeMouseInteractionEvent(*m_event);
    }

    void ImmediateModeActionDispatcher::KeyboardModifierDownImpl(const KeyboardModifier& keyModifier)
    {
        ToggleOn(GetMouseInteractionEvent()->m_mouseInteraction.m_keyboardModifiers.m_keyModifiers, keyModifier);
    }

    void ImmediateModeActionDispatcher::KeyboardModifierUpImpl(const KeyboardModifier& keyModifier)
    {
        ToggleOff(GetMouseInteractionEvent()->m_mouseInteraction.m_keyboardModifiers.m_keyModifiers, keyModifier);
    }

    void ImmediateModeActionDispatcher::SetEntityWorldTransformImpl(AZ::EntityId entityId, const AZ::Transform& transform)
    {
        AzToolsFramework::SetWorldTransform(entityId, transform);
    }

    void ImmediateModeActionDispatcher::SetSelectedEntityImpl(AZ::EntityId entity)
    {
        AzToolsFramework::SelectEntity(entity);
    }

    void ImmediateModeActionDispatcher::SetSelectedEntitiesImpl(const AzToolsFramework::EntityIdList& entities)
    {
        AzToolsFramework::SelectEntities(entities);
    }

    void ImmediateModeActionDispatcher::EnterComponentModeImpl(const AZ::Uuid& uuid)
    {
        using AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus;
        ComponentModeSystemRequestBus::Broadcast(&ComponentModeSystemRequestBus::Events::AddSelectedComponentModesOfType, uuid);
    }

    const AzToolsFramework::ViewportInteraction::MouseInteractionEvent* ImmediateModeActionDispatcher::GetMouseInteractionEvent() const
    {
        if (!m_event)
        {
            m_event = AZStd::unique_ptr<MouseInteractionEvent>(AZStd::make_unique<MouseInteractionEvent>());
            m_event->m_mouseInteraction.m_interactionId.m_viewportId =
                m_viewportManipulatorInteraction.GetViewportInteraction().GetViewportId();
        }

        return m_event.get();
    }

    AzToolsFramework::ViewportInteraction::MouseInteractionEvent* ImmediateModeActionDispatcher::GetMouseInteractionEvent()
    {
        return const_cast<MouseInteractionEvent*>(static_cast<const ImmediateModeActionDispatcher*>(this)->GetMouseInteractionEvent());
    }

    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ExpectTrue(bool result)
    {
        Log("Expecting true");
        EXPECT_TRUE(result);
        return this;
    }

    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ExpectFalse(bool result)
    {
        Log("Expecting false");
        EXPECT_FALSE(result);
        return this;
    }

    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::GetEntityWorldTransform(AZ::EntityId entityId, AZ::Transform& transform)
    {
        Log("Getting entity world transform");
        transform = AzToolsFramework::GetWorldTransform(entityId);
        return this;
    }

    void ImmediateModeActionDispatcher::ExpectManipulatorBeingInteractedImpl()
    {
        EXPECT_TRUE(m_viewportManipulatorInteraction.GetManipulatorManager().ManipulatorBeingInteracted());
    }

    void ImmediateModeActionDispatcher::ExpectManipulatorNotBeingInteractedImpl()
    {
        EXPECT_FALSE(m_viewportManipulatorInteraction.GetManipulatorManager().ManipulatorBeingInteracted());
    }

    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ResetEvent()
    {
        Log("Resetting the event state");
        m_event.reset();
        return this;
    }

    ImmediateModeActionDispatcher* ImmediateModeActionDispatcher::ExecuteBlock(const AZStd::function<void()>& blockFn)
    {
        blockFn();
        return this;
    }
} // namespace AzManipulatorTestFramework
