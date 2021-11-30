/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/TraceMessagesDrillerBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

namespace UnitTest
{
    /** Trace message handler to track messages during tests
    */
    struct PythonTraceMessageSink final
        : public AZ::Debug::TraceMessageDrillerBus::Handler
        , public AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
    {
        PythonTraceMessageSink()
        {
            AZ::Debug::TraceMessageDrillerBus::Handler::BusConnect();
            AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();
        }

        ~PythonTraceMessageSink()
        {
            AZ::Debug::TraceMessageDrillerBus::Handler::BusDisconnect();
            AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
        }

        // returns an index-tag for a message type that will be counted inside of m_evaluationMap
        using EvaluateMessageFunc = AZStd::function<int(const char* window, const char* message)>;
        EvaluateMessageFunc m_evaluateMessage;

        using EvaluationMap = AZStd::unordered_map<int, int>; // tag to count
        EvaluationMap m_evaluationMap;

        AZStd::mutex m_lock;

        void CleanUp()
        {
            AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
            m_evaluateMessage = {};
            m_evaluationMap.clear();
        }

        //////////////////////////////////////////////////////////////////////////
        // TraceMessageDrillerBus
        void OnPrintf(const char* window, const char* message) override
        {
            OnOutput(window, message);
        }

        void OnOutput(const char* window, const char* message) override
        {
            AZStd::lock_guard<decltype(m_lock)> lock(m_lock);

            if (m_evaluateMessage)
            {
                int key = m_evaluateMessage(window, message);
                if (key != 0)
                {
                    auto entryIt = m_evaluationMap.find(key);
                    if (m_evaluationMap.end() == entryIt)
                    {
                        m_evaluationMap[key] = 1;
                    }
                    else
                    {
                        m_evaluationMap[key] = m_evaluationMap[key] + 1;
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorPythonConsoleNotificationBus
        void OnTraceMessage([[maybe_unused]] AZStd::string_view message) override
        {
            AZ_TracePrintf("python", "%.*s", static_cast<int>(message.size()), message.data());
        }

        void OnErrorMessage([[maybe_unused]] AZStd::string_view message) override
        {
            AZ_Error("python", false, "%.*s", static_cast<int>(message.size()), message.data());
        }

        void OnExceptionMessage([[maybe_unused]] AZStd::string_view message) override
        {
            AZ_Error("python", false, "EXCEPTION: %.*s", static_cast<int>(message.size()), message.data());
        }

    };
}
