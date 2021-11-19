/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Channels/InputChannelDeltaWithSharedPosition2D.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedModifierKeyStates.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedPosition2D.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QEvent>
#include <QObject>
#include <QPoint>
#endif //! defined(Q_MOC_RUN)

class QWidget;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

namespace AzToolsFramework
{
    //! Maps events from the Qt input system to synthetic InputChannels in AzFramework
    //! that can be used by AzFramework::ViewportControllers.
    class QtEventToAzInputMapper final : public QObject
    {
        Q_OBJECT

    public:
        QtEventToAzInputMapper(QWidget* sourceWidget, int syntheticDeviceId = 0);
        ~QtEventToAzInputMapper() = default;

        //! Queries whether a given input channel has a synthetic equivalent mapped
        //! by this system.
        //! \returns true if the channel is handled by MapQtEventToAzInput.
        bool HandlesInputEvent(const AzFramework::InputChannel& channel) const;

        //! Sets whether or not this input mapper should be updating its input channels from Qt events.
        void SetEnabled(bool enabled);

        //! Sets whether or not the cursor should be constrained to the source widget and invisible.
        //! Internally, this will reset the cursor position after each move event to ensure movement
        //! events don't allow the cursor to escape. This can be used for typical camera controls
        //! like a dolly or rotation, where mouse movement is important but cursor location is not.
        void SetCursorCaptureEnabled(bool enabled);

        void SetOverrideCursor(ViewportInteraction::CursorStyleOverride cursorStyleOverride);
        void ClearOverrideCursor();

        // QObject overrides...
        bool eventFilter(QObject* object, QEvent* event) override;

    signals:
        //! This signal fires whenever the state of the specified input channel changes.
        //! This is determined by Qt events dispatched to the source widget.
        //! \param channel The AZ input channel that has been updated.
        //! \param event The underlying Qt event that triggered this change, if applicable.
        void InputChannelUpdated(const AzFramework::InputChannel* channel, QEvent* event);

    private:
        // Gets an input channel of the specified type by ID.
        template<class TInputChannel>
        TInputChannel* GetInputChannel(const AzFramework::InputChannelId& id)
        {
            auto channelIt = m_channels.find(id);
            if (channelIt != m_channels.end())
            {
                return static_cast<TInputChannel*>(channelIt->second);
            }
            return nullptr;
        }

        // Adds channels from the specified channel container to our input channel ID -> input channel lookup table.
        // Used for rapid lookup.
        template <class TContainer>
        void AddChannels(const TContainer& container)
        {
            for (const auto& channelData : container)
            {
                // Break const as we're taking these input channels from devices we own.
                m_channels.emplace(channelData.first, const_cast<AzFramework::InputChannel*>(channelData.second));
            }
        }

        // Our synthetic Keyboard device, does no internal keyboard handling and instead listens to this class for updates.
        class EditorQtKeyboardDevice : public AzFramework::InputDeviceKeyboard
        {
        public:
            EditorQtKeyboardDevice(AzFramework::InputDeviceId id);

            friend class QtEventToAzInputMapper;
        };

        // Our synthetic Mouse device, does no internal keyboard handling and instead listens to this class for updates.
        class EditorQtMouseDevice : public AzFramework::InputDeviceMouse
        {
        public:
            EditorQtMouseDevice(AzFramework::InputDeviceId id);

            // AzFramework::InputDeviceMouse overrides ...
            void SetSystemCursorState(AzFramework::SystemCursorState systemCursorState) override;
            AzFramework::SystemCursorState GetSystemCursorState() const override;

            friend class QtEventToAzInputMapper;

        private:
            AzFramework::SystemCursorState m_systemCursorState = AzFramework::SystemCursorState::UnconstrainedAndVisible;
        };

        // Emits InputChannelUpdated if channel has transitioned in state (i.e. has gone from active to inactive or vice versa).
        void NotifyUpdateChannelIfNotIdle(const AzFramework::InputChannel* channel, QEvent* event);

        // Processes any pending mouse movement events, this allows mouse movement channels to close themselves.
        void ProcessPendingMouseEvents(const QPoint& cursorDelta);

        // Converts a point in logical source widget space [0..m_sourceWidget->size()] to normalized [0..1] space.
        AZ::Vector2 WidgetPositionToNormalizedPosition(const QPoint& position);
        // Converts a point in normalized [0..1] space to logical source widget space [0..m_sourceWidget->size()].
        QPoint NormalizedPositionToWidgetPosition(const AZ::Vector2& normalizedPosition);

        // Handle mouse click events.
        void HandleMouseButtonEvent(QMouseEvent* mouseEvent);
        // Handle mouse move events.
        void HandleMouseMoveEvent(const QPoint& globalCursorPosition);
        // Handles key press / release events (or ShortcutOverride events for keys listed in m_highPriorityKeys).
        void HandleKeyEvent(QKeyEvent* keyEvent);
        // Handles mouse wheel events.
        void HandleWheelEvent(QWheelEvent* wheelEvent);

        // Clear all input channels (set all channel states to 'ended').
        void ClearInputChannels(QEvent* event);

        // Populates m_keyMappings.
        void InitializeKeyMappings();
        // Populates m_mouseButtonMappings.
        void InitializeMouseButtonMappings();
        // Populates m_highPriorityKeys.
        void InitializeHighPriorityKeys();

        // The current keyboard modifier state used by our synthetic key input channels.
        AZStd::shared_ptr<AzFramework::ModifierKeyStates> m_keyboardModifiers;
        // A lookup table for Qt key -> AZ input channel.
        AZStd::unordered_map<Qt::Key, AzFramework::InputChannelId> m_keyMappings;
        // A lookup table for Qt mouse button -> AZ input channel.
        AZStd::unordered_map<Qt::MouseButton, AzFramework::InputChannelId> m_mouseButtonMappings;
        // A set of high priority keys that need to be processed at the ShortcutOverride level instead of the
        // KeyEvent level. This prevents e.g. the main menu bar from processing a press of the "alt" key when the
        // viewport consumes the event.
        AZStd::unordered_set<Qt::Key> m_highPriorityKeys;
        // A lookup table for AZ input channel ID -> physical input channel on our mouse or keyboard device.
        AZStd::unordered_map<AzFramework::InputChannelId, AzFramework::InputChannel*> m_channels;
        // Where the mouse cursor was at the last cursor event.
        QPoint m_previousGlobalCursorPosition;
        // The source widget to map events from, used to calculate the relative mouse position within the widget bounds.
        QWidget* m_sourceWidget;
        // Flags whether or not Qt events should currently be processed.
        bool m_enabled = true;
        // Flags whether or not the cursor is being constrained to the source widget (for invisible mouse movement).
        bool m_capturingCursor = false;
        // Flags whether the cursor has been overridden.
        bool m_overrideCursor = false;

        // Our viewport-specific AZ devices. We control their internal input channel states.
        AZStd::unique_ptr<EditorQtMouseDevice> m_mouseDevice;
        AZStd::unique_ptr<EditorQtKeyboardDevice> m_keyboardDevice;
    };
} // namespace AzToolsFramework
