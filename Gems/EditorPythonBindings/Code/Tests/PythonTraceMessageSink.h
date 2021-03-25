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

        //////////////////////////////////////////////////////////////////////////
        // TraceMessageDrillerBus
        void OnPrintf(const char* window, const char* message) override
        {
            OnOutput(window, message);
        }

        void OnOutput(const char* window, const char* message) override
        {
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
