/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Input/QtEventToAzInputMapper.h>

#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Input/Buses/Notifications/InputChannelNotificationBus.h>
#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QWidget>

namespace AzToolsFramework
{
    void QtEventToAzInputMapper::InitializeKeyMappings()
    {
        // This assumes modifier keys (ctrl/shift/alt) map to the left control/shift/alt keys as Qt provides no way to disambiguate
        // in a platform agnostic manner. This could be expanded later with a PAL mapping from native scan codes acquired from
        // QKeyEvents, if needed.
        m_keyMappings = { {
            { Qt::Key_0, AzFramework::InputDeviceKeyboard::Key::Alphanumeric0 },
            { Qt::Key_1, AzFramework::InputDeviceKeyboard::Key::Alphanumeric1 },
            { Qt::Key_2, AzFramework::InputDeviceKeyboard::Key::Alphanumeric2 },
            { Qt::Key_3, AzFramework::InputDeviceKeyboard::Key::Alphanumeric3 },
            { Qt::Key_4, AzFramework::InputDeviceKeyboard::Key::Alphanumeric4 },
            { Qt::Key_5, AzFramework::InputDeviceKeyboard::Key::Alphanumeric5 },
            { Qt::Key_6, AzFramework::InputDeviceKeyboard::Key::Alphanumeric6 },
            { Qt::Key_7, AzFramework::InputDeviceKeyboard::Key::Alphanumeric7 },
            { Qt::Key_8, AzFramework::InputDeviceKeyboard::Key::Alphanumeric8 },
            { Qt::Key_9, AzFramework::InputDeviceKeyboard::Key::Alphanumeric9 },
            { Qt::Key_A, AzFramework::InputDeviceKeyboard::Key::AlphanumericA },
            { Qt::Key_B, AzFramework::InputDeviceKeyboard::Key::AlphanumericB },
            { Qt::Key_C, AzFramework::InputDeviceKeyboard::Key::AlphanumericC },
            { Qt::Key_D, AzFramework::InputDeviceKeyboard::Key::AlphanumericD },
            { Qt::Key_E, AzFramework::InputDeviceKeyboard::Key::AlphanumericE },
            { Qt::Key_F, AzFramework::InputDeviceKeyboard::Key::AlphanumericF },
            { Qt::Key_G, AzFramework::InputDeviceKeyboard::Key::AlphanumericG },
            { Qt::Key_H, AzFramework::InputDeviceKeyboard::Key::AlphanumericH },
            { Qt::Key_I, AzFramework::InputDeviceKeyboard::Key::AlphanumericI },
            { Qt::Key_J, AzFramework::InputDeviceKeyboard::Key::AlphanumericJ },
            { Qt::Key_K, AzFramework::InputDeviceKeyboard::Key::AlphanumericK },
            { Qt::Key_L, AzFramework::InputDeviceKeyboard::Key::AlphanumericL },
            { Qt::Key_M, AzFramework::InputDeviceKeyboard::Key::AlphanumericM },
            { Qt::Key_N, AzFramework::InputDeviceKeyboard::Key::AlphanumericN },
            { Qt::Key_O, AzFramework::InputDeviceKeyboard::Key::AlphanumericO },
            { Qt::Key_P, AzFramework::InputDeviceKeyboard::Key::AlphanumericP },
            { Qt::Key_Q, AzFramework::InputDeviceKeyboard::Key::AlphanumericQ },
            { Qt::Key_R, AzFramework::InputDeviceKeyboard::Key::AlphanumericR },
            { Qt::Key_S, AzFramework::InputDeviceKeyboard::Key::AlphanumericS },
            { Qt::Key_T, AzFramework::InputDeviceKeyboard::Key::AlphanumericT },
            { Qt::Key_U, AzFramework::InputDeviceKeyboard::Key::AlphanumericU },
            { Qt::Key_V, AzFramework::InputDeviceKeyboard::Key::AlphanumericV },
            { Qt::Key_W, AzFramework::InputDeviceKeyboard::Key::AlphanumericW },
            { Qt::Key_X, AzFramework::InputDeviceKeyboard::Key::AlphanumericX },
            { Qt::Key_Y, AzFramework::InputDeviceKeyboard::Key::AlphanumericY },
            { Qt::Key_Z, AzFramework::InputDeviceKeyboard::Key::AlphanumericZ },
            { Qt::Key_Backspace, AzFramework::InputDeviceKeyboard::Key::EditBackspace },
            { Qt::Key_CapsLock, AzFramework::InputDeviceKeyboard::Key::EditCapsLock },
            { Qt::Key_Enter, AzFramework::InputDeviceKeyboard::Key::EditEnter },
            { Qt::Key_Space, AzFramework::InputDeviceKeyboard::Key::EditSpace },
            { Qt::Key_Tab, AzFramework::InputDeviceKeyboard::Key::EditTab },
            { Qt::Key_Escape, AzFramework::InputDeviceKeyboard::Key::Escape },
            { Qt::Key_F1, AzFramework::InputDeviceKeyboard::Key::Function01 },
            { Qt::Key_F2, AzFramework::InputDeviceKeyboard::Key::Function02 },
            { Qt::Key_F3, AzFramework::InputDeviceKeyboard::Key::Function03 },
            { Qt::Key_F4, AzFramework::InputDeviceKeyboard::Key::Function04 },
            { Qt::Key_F5, AzFramework::InputDeviceKeyboard::Key::Function05 },
            { Qt::Key_F6, AzFramework::InputDeviceKeyboard::Key::Function06 },
            { Qt::Key_F7, AzFramework::InputDeviceKeyboard::Key::Function07 },
            { Qt::Key_F8, AzFramework::InputDeviceKeyboard::Key::Function08 },
            { Qt::Key_F9, AzFramework::InputDeviceKeyboard::Key::Function09 },
            { Qt::Key_F10, AzFramework::InputDeviceKeyboard::Key::Function10 },
            { Qt::Key_F11, AzFramework::InputDeviceKeyboard::Key::Function11 },
            { Qt::Key_F12, AzFramework::InputDeviceKeyboard::Key::Function12 },
            { Qt::Key_F13, AzFramework::InputDeviceKeyboard::Key::Function13 },
            { Qt::Key_F14, AzFramework::InputDeviceKeyboard::Key::Function14 },
            { Qt::Key_F15, AzFramework::InputDeviceKeyboard::Key::Function15 },
            { Qt::Key_F16, AzFramework::InputDeviceKeyboard::Key::Function16 },
            { Qt::Key_F17, AzFramework::InputDeviceKeyboard::Key::Function17 },
            { Qt::Key_F18, AzFramework::InputDeviceKeyboard::Key::Function18 },
            { Qt::Key_F19, AzFramework::InputDeviceKeyboard::Key::Function19 },
            { Qt::Key_F20, AzFramework::InputDeviceKeyboard::Key::Function20 },
            { Qt::Key_Alt, AzFramework::InputDeviceKeyboard::Key::ModifierAltL },
            { Qt::Key_Control, AzFramework::InputDeviceKeyboard::Key::ModifierCtrlL },
            { Qt::Key_Shift, AzFramework::InputDeviceKeyboard::Key::ModifierShiftL },
            { Qt::Key_Super_L, AzFramework::InputDeviceKeyboard::Key::ModifierSuperL },
            { Qt::Key_Super_R, AzFramework::InputDeviceKeyboard::Key::ModifierSuperR },
            { Qt::Key_Down, AzFramework::InputDeviceKeyboard::Key::NavigationArrowDown },
            { Qt::Key_Left, AzFramework::InputDeviceKeyboard::Key::NavigationArrowLeft },
            { Qt::Key_Right, AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight },
            { Qt::Key_Up, AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp },
            { Qt::Key_Delete, AzFramework::InputDeviceKeyboard::Key::NavigationDelete },
            { Qt::Key_End, AzFramework::InputDeviceKeyboard::Key::NavigationEnd },
            { Qt::Key_Home, AzFramework::InputDeviceKeyboard::Key::NavigationHome },
            { Qt::Key_Insert, AzFramework::InputDeviceKeyboard::Key::NavigationInsert },
            { Qt::Key_PageDown, AzFramework::InputDeviceKeyboard::Key::NavigationPageDown },
            { Qt::Key_PageUp, AzFramework::InputDeviceKeyboard::Key::NavigationPageUp },
            { Qt::Key_Apostrophe, AzFramework::InputDeviceKeyboard::Key::PunctuationApostrophe },
            { Qt::Key_Backslash, AzFramework::InputDeviceKeyboard::Key::PunctuationBackslash },
            { Qt::Key_BracketLeft, AzFramework::InputDeviceKeyboard::Key::PunctuationBracketL },
            { Qt::Key_BracketRight, AzFramework::InputDeviceKeyboard::Key::PunctuationBracketR },
            { Qt::Key_Comma, AzFramework::InputDeviceKeyboard::Key::PunctuationComma },
            { Qt::Key_Equal, AzFramework::InputDeviceKeyboard::Key::PunctuationEquals },
            { Qt::Key_hyphen, AzFramework::InputDeviceKeyboard::Key::PunctuationHyphen },
            { Qt::Key_Period, AzFramework::InputDeviceKeyboard::Key::PunctuationPeriod },
            { Qt::Key_Semicolon, AzFramework::InputDeviceKeyboard::Key::PunctuationSemicolon },
            { Qt::Key_Slash, AzFramework::InputDeviceKeyboard::Key::PunctuationSlash },
            { Qt::Key_QuoteLeft, AzFramework::InputDeviceKeyboard::Key::PunctuationTilde },
            { Qt::Key_Pause, AzFramework::InputDeviceKeyboard::Key::WindowsSystemPause },
            { Qt::Key_Print, AzFramework::InputDeviceKeyboard::Key::WindowsSystemPrint },
            { Qt::Key_ScrollLock, AzFramework::InputDeviceKeyboard::Key::WindowsSystemScrollLock },
        } };
    }

    void QtEventToAzInputMapper::InitializeMouseButtonMappings()
    {
        m_mouseButtonMappings = { {
            { Qt::MouseButton::LeftButton, AzFramework::InputDeviceMouse::Button::Left },
            { Qt::MouseButton::RightButton, AzFramework::InputDeviceMouse::Button::Right },
            { Qt::MouseButton::MiddleButton, AzFramework::InputDeviceMouse::Button::Middle },
            { Qt::MouseButton::ExtraButton1, AzFramework::InputDeviceMouse::Button::Other1 },
            { Qt::MouseButton::ExtraButton2, AzFramework::InputDeviceMouse::Button::Other2 },
        } };
    }

    // Currently this is only set for modifier keys.
    // This should only be expanded sparingly, any keys handled here will not be bubbled up to the shortcut system.
    // ex: If Key_S was here, the viewport would consume S key presses before the application could process a QAction with a Ctrl+S
    // shortcut.
    void QtEventToAzInputMapper::InitializeHighPriorityKeys()
    {
        m_highPriorityKeys = { Qt::Key_Alt, Qt::Key_Control, Qt::Key_Shift, Qt::Key_Super_L, Qt::Key_Super_R };
    }

    QtEventToAzInputMapper::EditorQtKeyboardDevice::EditorQtKeyboardDevice(AzFramework::InputDeviceId id)
        : AzFramework::InputDeviceKeyboard(id)
    {
        // Disable all platform native processing in favor of our Qt event handling
        SetImplementation(nullptr);
    }

    QtEventToAzInputMapper::EditorQtMouseDevice::EditorQtMouseDevice(AzFramework::InputDeviceId id)
        : AzFramework::InputDeviceMouse(id)
    {
        // Disable all platform native processing in favor of our Qt event handling
        SetImplementation(nullptr);
    }

    void QtEventToAzInputMapper::EditorQtMouseDevice::SetSystemCursorState(const AzFramework::SystemCursorState systemCursorState)
    {
        m_systemCursorState = systemCursorState;
    }

    AzFramework::SystemCursorState QtEventToAzInputMapper::EditorQtMouseDevice::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    QtEventToAzInputMapper::QtEventToAzInputMapper(QWidget* sourceWidget, int syntheticDeviceId)
        : QObject(sourceWidget)
        , m_sourceWidget(sourceWidget)
        , m_keyboardModifiers(AZStd::make_shared<AzFramework::ModifierKeyStates>())
    {
        InitializeKeyMappings();
        InitializeMouseButtonMappings();
        InitializeHighPriorityKeys();

        // Add an arbitrary offset to our device index to avoid collision with real physical device index.
        // We still have to use the keyboard and mouse device channel names because input channels are only addressed
        // by their own name and their device index, so overlapping input channels between devices would conflict.
        constexpr AZ::u32 syntheticDeviceOffset = 1000;
        const AzFramework::InputDeviceId keyboardDeviceId(
            AzFramework::InputDeviceKeyboard::Id.GetName(), syntheticDeviceId + syntheticDeviceOffset);
        const AzFramework::InputDeviceId mouseDeviceId(
            AzFramework::InputDeviceMouse::Id.GetName(), syntheticDeviceId + syntheticDeviceOffset);

        m_keyboardDevice = AZStd::make_unique<EditorQtKeyboardDevice>(keyboardDeviceId);
        m_mouseDevice = AZStd::make_unique<EditorQtMouseDevice>(mouseDeviceId);

        AddChannels(m_keyboardDevice->m_allChannelsById);
        AddChannels(m_mouseDevice->m_allChannelsById);

        // Install a global event filter to ensure we don't miss mouse and key release events.
        QApplication::instance()->installEventFilter(this);
    }

    bool QtEventToAzInputMapper::HandlesInputEvent(const AzFramework::InputChannel& channel) const
    {
        // We map keyboard and mouse events from Qt, so flag all events coming from those devices
        // as handled by our synthetic event system.
        const AzFramework::InputDeviceId& deviceId = channel.GetInputDevice().GetInputDeviceId();
        return deviceId.GetNameCrc32() == AzFramework::InputDeviceMouse::Id.GetNameCrc32() ||
            deviceId.GetNameCrc32() == AzFramework::InputDeviceKeyboard::Id.GetNameCrc32();
    }

    void QtEventToAzInputMapper::SetEnabled(bool enabled)
    {
        m_enabled = enabled;
        if (!enabled)
        {
            // Clear input channels to reset our input state if we're disabled.
            ClearInputChannels(nullptr);
        }
    }

    void QtEventToAzInputMapper::SetCursorCaptureEnabled(bool enabled)
    {
        if (m_capturingCursor != enabled)
        {
            m_capturingCursor = enabled;

            if (m_capturingCursor)
            {
                m_mouseDevice->SetSystemCursorState(AzFramework::SystemCursorState::ConstrainedAndHidden);
                qApp->setOverrideCursor(Qt::BlankCursor);
            }
            else
            {
                m_mouseDevice->SetSystemCursorState(AzFramework::SystemCursorState::UnconstrainedAndVisible);
                qApp->restoreOverrideCursor();
            }
        }
    }

    bool QtEventToAzInputMapper::eventFilter(QObject* object, QEvent* event)
    {
        // Abort if processing isn't enabled.
        if (!m_enabled)
        {
            return false;
        }

        const auto eventType = event->type();

        if (eventType == QEvent::Type::MouseMove)
        {
            // Clear override cursor when moving outside of the viewport
            const auto* mouseEvent = static_cast<const QMouseEvent*>(event);
            if (m_overrideCursor && !m_sourceWidget->geometry().contains(m_sourceWidget->mapFromGlobal(mouseEvent->globalPos())))
            {
                qApp->restoreOverrideCursor();
                m_overrideCursor = false;
            }
        }

        // If the application state changes (e.g. we have alt-tabbed or minimized the
        // main editor window) then ensure all input channels are cleared
        if (eventType == QEvent::ApplicationStateChange)
        {
            ClearInputChannels(event);
        }

        // Only accept mouse & key release events that originate from an object that is not our target widget,
        // as we don't want to erroneously intercept user input meant for another component.
        if (object != m_sourceWidget && eventType != QEvent::Type::KeyRelease && eventType != QEvent::Type::MouseButtonRelease)
        {
            return false;
        }

        if (eventType == QEvent::FocusIn || eventType == QEvent::FocusOut)
        {
            // If we focus in on the source widget and the mouse is contained in its
            // bounds, refresh the cached cursor position to ensure it is up to date (this
            // ensures cursor positions are refreshed correctly with context menu focus changes)
            if (eventType == QEvent::FocusIn)
            {
                const auto globalCursorPosition = QCursor::pos();
                if (m_sourceWidget->geometry().contains(m_sourceWidget->mapFromGlobal(globalCursorPosition)))
                {
                    HandleMouseMoveEvent(globalCursorPosition);
                }
            }
        }
        // Map key events to input channels.
        // ShortcutOverride is used in lieu of KeyPress for high priority input channels like Alt
        // that need to be accepted and stopped before they bubble up and cause unintended behavior.
        else if (
            eventType == QEvent::Type::KeyPress || eventType == QEvent::Type::KeyRelease || eventType == QEvent::Type::ShortcutOverride)
        {
            auto keyEvent = static_cast<QKeyEvent*>(event);
            HandleKeyEvent(keyEvent);
        }
        // Map mouse events to input channels.
        else if (
            eventType == QEvent::Type::MouseButtonPress || eventType == QEvent::Type::MouseButtonRelease ||
            eventType == QEvent::Type::MouseButtonDblClick)
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            HandleMouseButtonEvent(mouseEvent);
        }
        // Map mouse movement to the movement input channels.
        // This includes SystemCursorPosition alongside Movement::X and Movement::Y.
        else if (eventType == QEvent::Type::MouseMove)
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            HandleMouseMoveEvent(mouseEvent->globalPos());
        }
        // Map wheel events to the mouse Z movement channel.
        else if (eventType == QEvent::Type::Wheel)
        {
            auto wheelEvent = static_cast<QWheelEvent*>(event);
            HandleWheelEvent(wheelEvent);
        }

        return false;
    }

    void QtEventToAzInputMapper::NotifyUpdateChannelIfNotIdle(const AzFramework::InputChannel* channel, QEvent* event)
    {
        if (channel->GetState() != AzFramework::InputChannel::State::Idle)
        {
            emit InputChannelUpdated(channel, event);
        }
    }

    void QtEventToAzInputMapper::ProcessPendingMouseEvents(const QPoint& cursorDelta)
    {
        auto systemCursorChannel =
            GetInputChannel<AzFramework::InputChannelDeltaWithSharedPosition2D>(AzFramework::InputDeviceMouse::SystemCursorPosition);
        auto movementXChannel =
            GetInputChannel<AzFramework::InputChannelDeltaWithSharedPosition2D>(AzFramework::InputDeviceMouse::Movement::X);
        auto movementYChannel =
            GetInputChannel<AzFramework::InputChannelDeltaWithSharedPosition2D>(AzFramework::InputDeviceMouse::Movement::Y);
        auto mouseWheelChannel =
            GetInputChannel<AzFramework::InputChannelDeltaWithSharedPosition2D>(AzFramework::InputDeviceMouse::Movement::Z);

        systemCursorChannel->ProcessRawInputEvent(m_mouseDevice->m_cursorPositionData2D->m_normalizedPositionDelta.GetLength());
        movementXChannel->ProcessRawInputEvent(static_cast<float>(cursorDelta.x()));
        movementYChannel->ProcessRawInputEvent(static_cast<float>(cursorDelta.y()));
        mouseWheelChannel->ProcessRawInputEvent(0.0f);

        NotifyUpdateChannelIfNotIdle(systemCursorChannel, nullptr);
        NotifyUpdateChannelIfNotIdle(movementXChannel, nullptr);
        NotifyUpdateChannelIfNotIdle(movementYChannel, nullptr);
        NotifyUpdateChannelIfNotIdle(mouseWheelChannel, nullptr);
    }

    void QtEventToAzInputMapper::HandleMouseButtonEvent(QMouseEvent* mouseEvent)
    {
        const Qt::MouseButton button = mouseEvent->button();

        if (auto buttonIt = m_mouseButtonMappings.find(button); buttonIt != m_mouseButtonMappings.end())
        {
            auto buttonChannel = GetInputChannel<AzFramework::InputChannelDigitalWithSharedPosition2D>(buttonIt->second);

            if (buttonChannel)
            {
                if (mouseEvent->type() != QEvent::Type::MouseButtonRelease)
                {
                    buttonChannel->UpdateState(true);
                }
                else
                {
                    buttonChannel->UpdateState(false);
                }

                NotifyUpdateChannelIfNotIdle(buttonChannel, mouseEvent);
            }
        }
    }

    AZ::Vector2 QtEventToAzInputMapper::WidgetPositionToNormalizedPosition(const QPoint& position)
    {
        const float normalizedX = aznumeric_cast<float>(position.x()) / aznumeric_cast<float>(m_sourceWidget->width());
        const float normalizedY = aznumeric_cast<float>(position.y()) / aznumeric_cast<float>(m_sourceWidget->height());
        return AZ::Vector2{ normalizedX, normalizedY };
    }

    QPoint QtEventToAzInputMapper::NormalizedPositionToWidgetPosition(const AZ::Vector2& normalizedPosition)
    {
        const int denormalizedX = aznumeric_cast<int>(normalizedPosition.GetX() * m_sourceWidget->width());
        const int denormalizedY = aznumeric_cast<int>(normalizedPosition.GetY() * m_sourceWidget->height());
        return QPoint{ denormalizedX, denormalizedY };
    }

    void QtEventToAzInputMapper::HandleMouseMoveEvent(const QPoint& globalCursorPosition)
    {
        const QPoint cursorDelta = globalCursorPosition - m_previousGlobalCursorPosition;

        m_mouseDevice->m_cursorPositionData2D->m_normalizedPosition =
            WidgetPositionToNormalizedPosition(m_sourceWidget->mapFromGlobal(globalCursorPosition));
        m_mouseDevice->m_cursorPositionData2D->m_normalizedPositionDelta = WidgetPositionToNormalizedPosition(cursorDelta);

        ProcessPendingMouseEvents(cursorDelta);

        if (m_capturingCursor)
        {
            // Reset our cursor position to the previous point
            AzQtComponents::SetCursorPos(m_previousGlobalCursorPosition);
        }
        else
        {
            m_previousGlobalCursorPosition = globalCursorPosition;
        }
    }

    void QtEventToAzInputMapper::HandleKeyEvent(QKeyEvent* keyEvent)
    {
        // Ignore key repeat events, they're unrelated to actual physical button presses.
        if (keyEvent->isAutoRepeat())
        {
            return;
        }

        const Qt::Key key = static_cast<Qt::Key>(keyEvent->key());

        // For ShortcutEvent, only continue processing if we're in the HighPriorityKeys set.
        if (keyEvent->type() != QEvent::Type::ShortcutOverride || m_highPriorityKeys.find(key) != m_highPriorityKeys.end())
        {
            if (auto keyIt = m_keyMappings.find(key); keyIt != m_keyMappings.end())
            {
                auto keyChannel = GetInputChannel<AzFramework::InputChannelDigitalWithSharedModifierKeyStates>(keyIt->second);

                if (keyChannel)
                {
                    if (keyEvent->type() == QEvent::Type::KeyPress || keyEvent->type() == QEvent::Type::ShortcutOverride)
                    {
                        keyChannel->UpdateState(true);
                    }
                    else
                    {
                        keyChannel->UpdateState(false);
                    }

                    NotifyUpdateChannelIfNotIdle(keyChannel, keyEvent);
                }
            }
        }
    }

    void QtEventToAzInputMapper::HandleWheelEvent(QWheelEvent* wheelEvent)
    {
        auto cursorZChannel =
            GetInputChannel<AzFramework::InputChannelDeltaWithSharedPosition2D>(AzFramework::InputDeviceMouse::Movement::Z);
        const QPoint angleDelta = wheelEvent->angleDelta();
        // Check both angles, as the alt modifier can change the wheel direction.
        int wheelAngle = angleDelta.x();
        if (wheelAngle == 0)
        {
            wheelAngle = angleDelta.y();
        }
        cursorZChannel->ProcessRawInputEvent(aznumeric_cast<float>(wheelAngle));
        NotifyUpdateChannelIfNotIdle(cursorZChannel, wheelEvent);
    }

    void QtEventToAzInputMapper::ClearInputChannels(QEvent* event)
    {
        for (auto& channelData : m_channels)
        {
            // If resetting the input device changed the channel state, submit it to the mapped channel list for processing.
            if (channelData.second->IsActive())
            {
                channelData.second->UpdateState(false);
                NotifyUpdateChannelIfNotIdle(channelData.second, event);
            }
        }
    }

    static Qt::CursorShape QtCursorFromAzCursor(const ViewportInteraction::CursorStyleOverride cursorStyleOverride)
    {
        switch (cursorStyleOverride)
        {
        case ViewportInteraction::CursorStyleOverride::Forbidden:
            return Qt::ForbiddenCursor;
        default:
            return Qt::ArrowCursor;
        }
    }

    void QtEventToAzInputMapper::SetOverrideCursor(ViewportInteraction::CursorStyleOverride cursorStyleOverride)
    {
        ClearOverrideCursor();

        qApp->setOverrideCursor(QtCursorFromAzCursor(cursorStyleOverride));
        m_overrideCursor = true;
    }

    void QtEventToAzInputMapper::ClearOverrideCursor()
    {
        if (m_overrideCursor)
        {
            qApp->restoreOverrideCursor();
            m_overrideCursor = false;
        }
    }
} // namespace AzToolsFramework
