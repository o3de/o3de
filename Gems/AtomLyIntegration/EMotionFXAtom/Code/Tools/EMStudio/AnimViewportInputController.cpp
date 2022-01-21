/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>

#include <EMStudio/AnimViewportInputController.h>


namespace EMStudio
{
    // static const auto ManipulatorPriority = AzFramework::ViewportControllerPriority::Highest;

    AnimViewportInputController::AnimViewportInputController()
        : AzFramework::SingleViewportController()
    {
    }

    AnimViewportInputController::~AnimViewportInputController()
    {
    }

    void AnimViewportInputController::Init(AZ::EntityId targetEntityId)
    {
        m_targetEntityId = targetEntityId;
    }

    AzToolsFramework::ViewportInteraction::MouseButton AnimViewportInputController::GetMouseButton(
        const AzFramework::InputChannel& inputChannel)
    {
        using AzToolsFramework::ViewportInteraction::MouseButton;
        using InputButton = AzFramework::InputDeviceMouse::Button;
        const auto& id = inputChannel.GetInputChannelId();
        if (id == InputButton::Left)
        {
            return MouseButton::Left;
        }
        if (id == InputButton::Middle)
        {
            return MouseButton::Middle;
        }
        if (id == InputButton::Right)
        {
            return MouseButton::Right;
        }
        return MouseButton::None;
    }

    bool AnimViewportInputController::IsMouseMove(const AzFramework::InputChannel& inputChannel)
    {
        return inputChannel.GetInputChannelId() == AzFramework::InputDeviceMouse::SystemCursorPosition;
    }

    AzToolsFramework::ViewportInteraction::KeyboardModifier AnimViewportInputController::GetKeyboardModifier(
        const AzFramework::InputChannel& inputChannel)
    {
        using AzToolsFramework::ViewportInteraction::KeyboardModifier;
        using Key = AzFramework::InputDeviceKeyboard::Key;
        const auto& id = inputChannel.GetInputChannelId();
        if (id == Key::ModifierAltL || id == Key::ModifierAltR)
        {
            return KeyboardModifier::Alt;
        }
        if (id == Key::ModifierCtrlL || id == Key::ModifierCtrlR)
        {
            return KeyboardModifier::Ctrl;
        }
        if (id == Key::ModifierShiftL || id == Key::ModifierShiftR)
        {
            return KeyboardModifier::Shift;
        }
        return KeyboardModifier::None;
    }

    bool AnimViewportInputController::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        // We only care about manipulator events
        //if (event.m_priority != ManipulatorPriority)
        //{
        //    return false;
        //}

        using AzFramework::InputChannel;
        using AzToolsFramework::ViewportInteraction::KeyboardModifier;
        using AzToolsFramework::ViewportInteraction::MouseButton;
        using AzToolsFramework::ViewportInteraction::MouseEvent;
        using AzToolsFramework::ViewportInteraction::MouseInteraction;
        using AzToolsFramework::ViewportInteraction::MouseInteractionEvent;
        using AzToolsFramework::ViewportInteraction::ProjectedViewportRay;
        using AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus;

        bool interactionHandled = false;
        float wheelDelta = 0.0f;
        AZStd::optional<MouseButton> overrideButton;
        AZStd::optional<MouseEvent> eventType;

        const auto state = event.m_inputChannel.GetState();
        if (IsMouseMove(event.m_inputChannel))
        {
            // Cache the ray trace results when doing manipulator interaction checks, no need to recalculate after
            // if (event.m_priority == ManipulatorPriority)
            {
                const auto* position = event.m_inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
                AZ_Assert(position, "Expected PositionData2D but found nullptr");

                AzFramework::WindowSize windowSize;
                AzFramework::WindowRequestBus::EventResult(
                    windowSize, event.m_windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);

                const auto screenPoint = AzFramework::ScreenPoint(
                    aznumeric_cast<int>(position->m_normalizedPosition.GetX() * windowSize.m_width),
                    aznumeric_cast<int>(position->m_normalizedPosition.GetY() * windowSize.m_height));

                ProjectedViewportRay ray{};
                ViewportInteractionRequestBus::EventResult(
                    ray, GetViewportId(), &ViewportInteractionRequestBus::Events::ViewportScreenToWorldRay, screenPoint);

                m_mouseInteraction.m_mousePick.m_rayOrigin = ray.m_origin;
                m_mouseInteraction.m_mousePick.m_rayDirection = ray.m_direction;
                m_mouseInteraction.m_mousePick.m_screenCoordinates = screenPoint;
            }

            eventType = MouseEvent::Move;
        }
        else if (auto mouseButton = GetMouseButton(event.m_inputChannel); mouseButton != MouseButton::None)
        {
            const AZ::u32 mouseButtonValue = static_cast<AZ::u32>(mouseButton);
            overrideButton = mouseButton;
            if (state == InputChannel::State::Began)
            {
                m_mouseInteraction.m_mouseButtons.m_mouseButtons |= mouseButtonValue;
                eventType = MouseEvent::Down;
            }
            else if (state == InputChannel::State::Ended)
            {
                // If we've actually logged a mouse down event, forward a mouse up event.
                // This prevents corner cases like the context menu thinking it should be opened even though no one clicked in this
                // viewport, due to RenderViewportWidget ensuring all controllers get InputChannel::State::Ended events.
                if (m_mouseInteraction.m_mouseButtons.m_mouseButtons & mouseButtonValue)
                {
                    eventType = MouseEvent::Up;
                }
            }
        }
        else if (auto keyboardModifier = GetKeyboardModifier(event.m_inputChannel); keyboardModifier != KeyboardModifier::None)
        {
            if (state == InputChannel::State::Began || state == InputChannel::State::Updated)
            {
                m_mouseInteraction.m_keyboardModifiers.m_keyModifiers |= static_cast<AZ::u32>(keyboardModifier);
            }
            else if (state == InputChannel::State::Ended)
            {
                m_mouseInteraction.m_keyboardModifiers.m_keyModifiers &= ~static_cast<AZ::u32>(keyboardModifier);
            }
        }
        else if (event.m_inputChannel.GetInputChannelId() == AzFramework::InputDeviceMouse::Movement::Z)
        {
            if (state == InputChannel::State::Began || state == InputChannel::State::Updated)
            {
                eventType = MouseEvent::Wheel;
                wheelDelta = event.m_inputChannel.GetValue();
            }
        }

        if (eventType)
        {
            MouseInteraction mouseInteraction = m_mouseInteraction;
            if (overrideButton)
            {
                mouseInteraction.m_mouseButtons.m_mouseButtons = static_cast<AZ::u32>(overrideButton.value());
            }

            mouseInteraction.m_interactionId.m_viewportId = GetViewportId();

            auto currentCursorState = AzFramework::SystemCursorState::Unknown;
            AzFramework::InputSystemCursorRequestBus::EventResult(
                currentCursorState, event.m_inputChannel.GetInputDevice().GetInputDeviceId(),
                &AzFramework::InputSystemCursorRequestBus::Events::GetSystemCursorState);

            const auto mouseInteractionEvent = [mouseInteraction, event = eventType.value(), wheelDelta,
                                                cursorCaptured = currentCursorState == AzFramework::SystemCursorState::ConstrainedAndHidden]
            {
                switch (event)
                {
                case MouseEvent::Up:
                case MouseEvent::Down:
                case MouseEvent::Move:
                case MouseEvent::DoubleClick:
                    return MouseInteractionEvent(AZStd::move(mouseInteraction), event, cursorCaptured);
                case MouseEvent::Wheel:
                    return MouseInteractionEvent(AZStd::move(mouseInteraction), wheelDelta);
                }

                AZ_Assert(false, "Unhandled MouseEvent");
                return MouseInteractionEvent(MouseInteraction{}, MouseEvent::Up, false);
            }();

            // AzFramework::EntityContextId entityContextId = mouseInteraction.m_interactionId.m_entityContextId;
            interactionHandled = HandleMouseMove(mouseInteractionEvent);
        }

        return interactionHandled;
    }

    bool AnimViewportInputController::HandleMouseMove(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent)
    {
        if (!m_manipulatorManager)
        {
            return false;
        }

        using AzToolsFramework::ViewportInteraction::MouseEvent;
        const auto& mouseInteraction = mouseInteractionEvent.m_mouseInteraction;

        switch (mouseInteractionEvent.m_mouseEvent)
        {
        case MouseEvent::Down:
            return m_manipulatorManager->ConsumeViewportMousePress(mouseInteraction);
        case MouseEvent::DoubleClick:
            return false;
        case MouseEvent::Move:
            {
                const AzToolsFramework::ManipulatorManager::ConsumeMouseMoveResult mouseMoveResult =
                    m_manipulatorManager->ConsumeViewportMouseMove(mouseInteraction);
                return mouseMoveResult == AzToolsFramework::ManipulatorManager::ConsumeMouseMoveResult::Interacting;
            }
        case MouseEvent::Up:
            return m_manipulatorManager->ConsumeViewportMouseRelease(mouseInteraction);
        case MouseEvent::Wheel:
            return m_manipulatorManager->ConsumeViewportMouseWheel(mouseInteraction);
        default:
            return false;
        }
    }
} // namespace EMStudio
