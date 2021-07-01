/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Events/InputChannelEventListener.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener()
        : m_filter()
        , m_priority(GetPriorityDefault())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(bool autoConnect)
        : m_filter()
        , m_priority(GetPriorityDefault())
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZ::s32 priority)
        : m_filter()
        , m_priority(priority)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZ::s32 priority, bool autoConnect)
        : m_filter()
        , m_priority(priority)
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter)
        : m_filter(filter)
        , m_priority(GetPriorityDefault())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter,
                                                         AZ::s32 priority)
        : m_filter(filter)
        , m_priority(priority)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter,
                                                         AZ::s32 priority,
                                                         bool autoConnect)
        : m_filter(filter)
        , m_priority(priority)
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::s32 InputChannelEventListener::GetPriority() const
    {
        return m_priority;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::SetFilter(AZStd::shared_ptr<InputChannelEventFilter> filter)
    {
        m_filter = filter;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::Connect()
    {
        InputChannelNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::Disconnect()
    {
        InputChannelNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::OnInputChannelEvent(const InputChannel& inputChannel,
                                                        bool& o_hasBeenConsumed)
    {
        if (o_hasBeenConsumed)
        {
            return;
        }

        if (m_filter)
        {
            const bool doesPassFilter = m_filter->DoesPassFilter(inputChannel);
            if (!doesPassFilter)
            {
                return;
            }
        }

        o_hasBeenConsumed = OnInputChannelEventFiltered(inputChannel);
    }
} // namespace AzFramework
