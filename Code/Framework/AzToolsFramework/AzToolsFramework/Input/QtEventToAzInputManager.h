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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedModifierKeyStates.h>
#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedPosition2D.h>
#include <AzFramework/Input/Channels/InputChannelDeltaWithSharedPosition2D.h>

#include <QEvent>

class QWidget;

namespace AzToolsFramework
{
    //! Maps events from the Qt input system to synthetic InputChannels in AzFramework
    //! that can be used by AzFramework::ViewportControllers.
    class QtEventToAzInputMapper final
    {
    public:
        QtEventToAzInputMapper(QWidget* sourceWidget);
        ~QtEventToAzInputMapper() = default;

        //! Maps a Qt event to any relevant input channels
        //! \returns A vector containing all InputChannels that have changed state.
        AZStd::vector<AzFramework::InputChannel*> MapQtEventToAzInput(QEvent* event);
        //! Queries whether a given input channel has a synthetic equivalent mapped
        //! by this system.
        //! \returns true if the channel is handled by MapQtEventToAzInput.
        bool HandlesInputEvent(const AzFramework::InputChannel& channel) const;

    private:
        template <class TInputChannel>
        TInputChannel* GetInputChannel(const AzFramework::InputChannelId& id)
        {
            auto channelIt = m_channels.find(id);
            if (channelIt != m_channels.end())
            {
                return static_cast<TInputChannel*>(channelIt->second.get());
            }
            return {};
        }

        // Mapping from Qt::Keys to InputChannelIds.
        // Used to populate m_keyMappings.
        static AZStd::array<AZStd::pair<Qt::Key, AzFramework::InputChannelId>, 91> QtKeyMappings;
        // Mapping from Qt::MouseButtons to InputChannelIds.
        // Used to populate m_mouseButtonMappings.
        static AZStd::array<AZStd::pair<Qt::MouseButton, AzFramework::InputChannelId>, 5> QtMouseButtonMappings;
        // A set of high priority keys that need to be processed at the ShortcutOverride level instead of the
        // KeyEvent level. This prevents e.g. the main menu bar from processing a press of the "alt" key when the
        // viewport consumes the event.
        static AZStd::array<Qt::Key, 5> HighPriorityKeys;

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
