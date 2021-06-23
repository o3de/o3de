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

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Channels/InputChannelDeltaWithSharedPosition2D.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedModifierKeyStates.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedPosition2D.h>

#include <QEvent>
#include <QObject>
#endif //!defined(Q_MOC_RUN)

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
        QtEventToAzInputMapper(QWidget* sourceWidget);
        ~QtEventToAzInputMapper() = default;

        //! Queries whether a given input channel has a synthetic equivalent mapped
        //! by this system.
        //! \returns true if the channel is handled by MapQtEventToAzInput.
        bool HandlesInputEvent(const AzFramework::InputChannel& channel) const;

        // QObject overrides...
        bool eventFilter(QObject* object, QEvent* event) override;

    signals:
        //! This signal fires whenever the state of the specified input channel changes.
        //! This is determined by Qt events dispatched to the source widget.
        //! \param channel The AZ input channel that has been updated.
        //! \param event The underlying Qt event that triggered this change, if applicable.
        void InputChannelUpdated(const AzFramework::InputChannel* channel, QEvent* event);

    private:
        template<class TInputChannel>
        TInputChannel* GetInputChannel(const AzFramework::InputChannelId& id)
        {
            auto channelIt = m_channels.find(id);
            if (channelIt != m_channels.end())
            {
                return static_cast<TInputChannel*>(channelIt->second.get());
            }
            return {};
        }

        void NotifyUpdateChannelIfNotIdle(const AzFramework::InputChannel* channel, QEvent* event);

        void ProcessPendingMouseEvents();

        void HandleMouseButtonEvent(QMouseEvent* mouseEvent);
        void HandleMouseMoveEvent(QMouseEvent* mouseEvent);
        void HandleKeyEvent(QKeyEvent* keyEvent);
        void HandleWheelEvent(QWheelEvent* wheelEvent);
        void HandleFocusChange(QEvent* event);

        // The current keyboard modifier state used by our synthetic key input channels.
        AZStd::shared_ptr<AzFramework::ModifierKeyStates> m_keyboardModifiers;
        // The current normalized cursor position used by our synthetic system cursor event.
        AZStd::shared_ptr<AzFramework::InputChannel::PositionData2D> m_cursorPosition;
        // A lookup table for Qt key -> AZ input channel.
        AZStd::unordered_map<Qt::Key, AzFramework::InputChannelId> m_keyMappings;
        // A lookup table for Qt mouse button -> AZ input channel.
        AZStd::unordered_map<Qt::MouseButton, AzFramework::InputChannelId> m_mouseButtonMappings;
        // Our set of synthetic input channels that can be mapped by MapQtEventToAzInput.
        // These channels do not broadcast state changes to the actual AZ input devices, as we're bypassing
        // that system to integrate with Qt to allow coexistence with the platform native implementation.
        AZStd::unordered_map<AzFramework::InputChannelId, AZStd::unique_ptr<AzFramework::InputChannel>> m_channels;
        // The source widget to map events from, used to calculate the relative mouse position within the widget bounds.
        QWidget* m_sourceWidget;
        // Flags when mouse movement channels have been opened and may need to be closed (as there are no movement ended events).
        bool m_mouseChannelsNeedUpdate = false;
    };
} // namespace AzToolsFramework
