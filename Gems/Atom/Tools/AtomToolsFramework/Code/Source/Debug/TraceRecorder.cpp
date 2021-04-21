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

#include <AtomToolsFramework/Debug/TraceRecorder.h>

namespace AtomToolsFramework
{
    TraceRecorder::TraceRecorder()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    TraceRecorder::~TraceRecorder()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool TraceRecorder::OnAssert(const char* message)
    {
        m_messageSink += "Assert: ";
        m_messageSink += message;
        m_messageSink += "\n";
        return false;
    }

    bool TraceRecorder::OnException(const char* message)
    {
        m_messageSink += "Exception: ";
        m_messageSink += message;
        m_messageSink += "\n";
        return false;
    }

    bool TraceRecorder::OnError(const char* /*window*/, const char* message)
    {
        m_messageSink += "Error: ";
        m_messageSink += message;
        m_messageSink += "\n";
        return false;
    }

    bool TraceRecorder::OnWarning(const char* /*window*/, const char* message)
    {
        m_messageSink += "Warning: ";
        m_messageSink += message;
        m_messageSink += "\n";
        return false;
    }

    bool TraceRecorder::OnPrintf(const char* /*window*/, const char* message)
    {
        m_messageSink += message;
        m_messageSink += "\n";
        return false;
    }

} // namespace AtomToolsFramework
