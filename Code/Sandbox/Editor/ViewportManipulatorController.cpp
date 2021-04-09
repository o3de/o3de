/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ViewportManipulatorController.h"

#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzCore/Script/ScriptTimePoint.h>

#include <QApplication>

static const auto ManipulatorPriority = AzFramework::ViewportControllerPriority::High;
static const auto InteractionPriority = AzFramework::ViewportControllerPriority::Low;

namespace SandboxEditor
{

ViewportManipulatorControllerInstance::ViewportManipulatorControllerInstance(AzFramework::ViewportId viewport)
    : AzFramework::MultiViewportControllerInstanceInterface(viewport)
{
}

AzToolsFramework::ViewportInteraction::MouseButton ViewportManipulatorControllerInstance::GetMouseButton(
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

bool ViewportManipulatorControllerInstance::IsMouseMove(const AzFramework::InputChannel& inputChannel)
{
    return inputChannel.GetInputChannelId() == AzFramework::InputDeviceMouse::SystemCursorPosition;
}

AzToolsFramework::ViewportInteraction::KeyboardModifier ViewportManipulatorControllerInstance::GetKeyboardModifier(
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

bool ViewportManipulatorControllerInstance::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
{
    // We only care about manipulator and viewport interaction events
    if (event.m_priority != ManipulatorPriority && event.m_priority != InteractionPriority)
    {
        return false;
    }

    using InteractionBus = AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;
    using namespace AzToolsFramework::ViewportInteraction;
    using AzFramework::InputChannel;

    bool interactionHandled = false;
    AZStd::optional<MouseButton> overrideButton;
    AZStd::optional<MouseEvent> eventType;

    if (IsMouseMove(event.m_inputChannel))
    {
        // Cache the ray trace results when doing manipulator interaction checks, no need to recalculate after
        if (event.m_priority == ManipulatorPriority)
        {
            AzFramework::ScreenPoint screenPosition = AzFramework::ScreenPoint(0, 0);
            ViewportMouseCursorRequestBus::EventResult(
                screenPosition, GetViewportId(), &ViewportMouseCursorRequestBus::Events::ViewportCursorScreenPosition);

            m_state.m_mousePick.m_screenCoordinates = screenPosition;
            AZStd::optional<ProjectedViewportRay> ray;
            ViewportInteractionRequestBus::EventResult(
                ray, GetViewportId(), &ViewportInteractionRequestBus::Events::ViewportScreenToWorldRay,
                QPoint(screenPosition.m_x, screenPosition.m_y));

            if (ray.has_value())
            {
                m_state.m_mousePick.m_rayOrigin = ray.value().origin;
                m_state.m_mousePick.m_rayDirection = ray.value().direction;
            }
        }
        eventType = MouseEvent::Move;
    }
    else if (auto mouseButton = GetMouseButton(event.m_inputChannel); mouseButton != MouseButton::None)
    {
        overrideButton = mouseButton;
        if (event.m_inputChannel.GetState() == InputChannel::State::Began)
        {
            m_state.m_mouseButtons.m_mouseButtons |= static_cast<AZ::u32>(mouseButton);
            if (IsDoubleClick(mouseButton))
            {
                m_pendingDoubleClicks.erase(mouseButton);
                eventType = MouseEvent::DoubleClick;
            }
            else
            {
                m_pendingDoubleClicks[mouseButton] = m_curTime;
                eventType = MouseEvent::Down;
            }
        }
        else if (event.m_inputChannel.GetState() == InputChannel::State::Ended)
        {
            m_state.m_mouseButtons.m_mouseButtons &= ~static_cast<AZ::u32>(mouseButton);
            eventType = MouseEvent::Up;
        }
    }
    else if (auto keyboardModifier = GetKeyboardModifier(event.m_inputChannel); keyboardModifier != KeyboardModifier::None)
    {
        if (event.m_inputChannel.GetState() == InputChannel::State::Began || event.m_inputChannel.GetState() == InputChannel::State::Updated)
        {
            m_state.m_keyboardModifiers.m_keyModifiers |= static_cast<AZ::u32>(keyboardModifier);
        }
        else if (event.m_inputChannel.GetState() == InputChannel::State::Ended)
        {
            m_state.m_keyboardModifiers.m_keyModifiers &= ~static_cast<AZ::u32>(keyboardModifier);
        }
    }

    if (eventType)
    {
        MouseInteraction mouseInteraction = m_state;
        if (overrideButton)
        {
            mouseInteraction.m_mouseButtons.m_mouseButtons = static_cast<AZ::u32>(overrideButton.value());
        }
        MouseInteractionEvent mouseEvent = MouseInteractionEvent(mouseInteraction, eventType.value());

        // Depending on priority, we dispatch to either the manipulator or viewport interaction event
        const auto& targetInteractionEvent =
            event.m_priority == ManipulatorPriority
            ? &InteractionBus::Events::InternalHandleMouseManipulatorInteraction
            : &InteractionBus::Events::InternalHandleMouseViewportInteraction;

        InteractionBus::EventResult(
            interactionHandled,
            AzToolsFramework::GetEntityContextId(),
            targetInteractionEvent,
            MouseInteractionEvent(AZStd::move(mouseInteraction), eventType.value()));
    }

    return interactionHandled;
}

void ViewportManipulatorControllerInstance::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
{
    m_curTime = event.m_time;
}

bool ViewportManipulatorControllerInstance::IsDoubleClick(AzToolsFramework::ViewportInteraction::MouseButton button) const
{
    auto clickIt = m_pendingDoubleClicks.find(button);
    if (clickIt == m_pendingDoubleClicks.end())
    {
        return false;
    }
    const double doubleClickThresholdMilliseconds = qApp->doubleClickInterval();
    return (m_curTime.GetMilliseconds() - clickIt->second.GetMilliseconds()) < doubleClickThresholdMilliseconds;
}

} //namespace SandboxEditor
