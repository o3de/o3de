/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Events/InputChannelEventSink.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventSink::InputChannelEventSink()
        : m_filter()
    {
        InputChannelNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventSink::~InputChannelEventSink()
    {
        InputChannelNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventSink::InputChannelEventSink(AZStd::shared_ptr<InputChannelEventFilter> filter)
        : m_filter(filter)
    {
        InputChannelNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::s32 InputChannelEventSink::GetPriority() const
    {
        return std::numeric_limits<AZ::s32>::max();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventSink::SetFilter(AZStd::shared_ptr<InputChannelEventFilter> filter)
    {
        m_filter = filter;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventSink::OnInputChannelEvent(const InputChannel& inputChannel,
                                                    bool& o_hasBeenConsumed)
    {
        o_hasBeenConsumed = m_filter ? m_filter->DoesPassFilter(inputChannel) : true;
    }
} // namespace AzFramework
