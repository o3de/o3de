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

#include <AzFramework/Input/Events/InputTextEventListener.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputTextEventListener::InputTextEventListener()
        : m_priority(GetPriorityDefault())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputTextEventListener::InputTextEventListener(bool autoConnect)
        : m_priority(GetPriorityDefault())
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputTextEventListener::InputTextEventListener(AZ::s32 priority)
        : m_priority(priority)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputTextEventListener::InputTextEventListener(AZ::s32 priority, bool autoConnect)
        : m_priority(priority)
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::s32 InputTextEventListener::GetPriority() const
    {
        return m_priority;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputTextEventListener::Connect()
    {
        InputTextNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputTextEventListener::Disconnect()
    {
        InputTextNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputTextEventListener::OnInputTextEvent(const AZStd::string& textUTF8, bool& o_hasBeenConsumed)
    {
        if (!o_hasBeenConsumed)
        {
            o_hasBeenConsumed = OnInputTextEventFiltered(textUTF8);
        }
    }
} // namespace AzFramework
