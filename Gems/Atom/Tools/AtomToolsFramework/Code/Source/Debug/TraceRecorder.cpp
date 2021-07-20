/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Debug/TraceRecorder.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AtomToolsFramework
{
    TraceRecorder::TraceRecorder(size_t maxMessageCount)
        : m_maxMessageCount(maxMessageCount)
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    TraceRecorder::~TraceRecorder()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    AZStd::string TraceRecorder::GetDump() const
    {
        AZStd::string dump;
        AZ::StringFunc::Join(dump, m_messages.begin(), m_messages.end(), "\n");
        return dump;
    }

    bool TraceRecorder::OnAssert(const char* message)
    {
        if (m_messages.size() < m_maxMessageCount)
        {
            m_messages.push_back(AZStd::string::format("Assert: %s", message));
        }
        return false;
    }

    bool TraceRecorder::OnException(const char* message)
    {
        if (m_messages.size() < m_maxMessageCount)
        {
            m_messages.push_back(AZStd::string::format("Exception: %s", message));
        }
        return false;
    }

    bool TraceRecorder::OnError(const char* /*window*/, const char* message)
    {
        if (m_messages.size() < m_maxMessageCount)
        {
            m_messages.push_back(AZStd::string::format("Error: %s", message));
        }
        return false;
    }

    bool TraceRecorder::OnWarning(const char* /*window*/, const char* message)
    {
        if (m_messages.size() < m_maxMessageCount)
        {
            m_messages.push_back(AZStd::string::format("Warning: %s", message));
        }
        return false;
    }

    bool TraceRecorder::OnPrintf(const char* /*window*/, const char* message)
    {
        if (m_messages.size() < m_maxMessageCount)
        {
            m_messages.push_back(AZStd::string::format("%s", message));
        }
        return false;
    }

} // namespace AtomToolsFramework
