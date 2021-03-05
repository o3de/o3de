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
