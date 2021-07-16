/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Notifications/InputTextNotificationBus.h>
#include <AzFramework/Input/Channels/InputChannelId.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Function template that processes a generic raw input event queue, given an InputChannelClass
    //! that defines a ProcessRawInputEvent function that takes a RawEventType as the only parameter.
    //! \param[in] rawEventQueuesById A map (keyed by id) of raw input event queues.
    //! \param[in] inputChannelsById A map (keyed by id) of the input channels to potentially update.
    template<class InputChannelClass, typename RawEventType>
    inline void ProcessRawInputEventQueues(
        AZStd::unordered_map<InputChannelId, AZStd::vector<RawEventType>>& rawEventQueuesById,
        const AZStd::unordered_map<InputChannelId, InputChannelClass*>& inputChannelsById)
    {
        auto&& it = rawEventQueuesById.begin();
        while (it != rawEventQueuesById.end())
        {
            const InputChannelId& channelId = it->first;
            const auto& channelIt = inputChannelsById.find(channelId);
            if (channelIt == inputChannelsById.end() || !channelIt->second)
            {
                // Unknown channel id, warn but handle gracefully
                AZ_Warning("ProcessRawInputEventQueues", false,
                           "Raw input event queued with unrecognized id: %s", channelId.GetName());
                rawEventQueuesById.erase(it++);
                continue;
            }

            InputChannelClass& channel = *(channelIt->second);
            AZStd::vector<RawEventType>& rawEventQueue = it->second;
            if (!rawEventQueue.empty())
            {
                // Update the input channel once for each raw event queued since the last frame,
                // then clear the event queue so it can receive new events over the next frame.
                for (const RawEventType& rawEvent : rawEventQueue)
                {
                    channel.ProcessRawInputEvent(rawEvent);
                }
                rawEventQueue.clear();
            }
            else
            {
                // No raw input was received for this channel since the last frame, but we must
                // still update it to trigger state transitions and ensure that events are sent.
                // If this channel entered the 'Ended' state last frame (and it has not received
                // new raw input this frame) this update will cause it to enter the 'Idle' state.
                channel.UpdateState(channel.IsActive());
            }

            if (channel.IsStateIdle())
            {
                // When a channel returns to the idle state, removing its corresponding event queue
                // map entry conveniently allows the map to double as the set of non-idle channels.
                // This allows us to continue updating all non-idle channels (above) without having
                // to iterate over every channel every frame, as the majority of them will be idle.
                rawEventQueuesById.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Utility function that processes a queue of raw input text events.
    //! \param[in] rawTextEventQueue The queue of raw text events (all encoded using UTF-8)
    inline void ProcessRawInputTextEventQueue(AZStd::vector<AZStd::string>& rawTextEventQueue)
    {
        for (const AZStd::string& rawTextEvent : rawTextEventQueue)
        {
            bool hasBeenConsumed = false;
            InputTextNotificationBus::Broadcast(&InputTextNotifications::OnInputTextEvent,
                                                rawTextEvent,
                                                hasBeenConsumed);
        }
        rawTextEventQueue.clear();
    }
} // namespace AzFramework
