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

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

namespace PythonBindingsExample
{
    struct ApplicationParameters;

    class Application
        : public AzToolsFramework::ToolsApplication
        , protected AZ::Debug::TraceMessageBus::Handler
        , protected AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
    {
    public:
        Application(int* argc, char*** argv);
        ~Application();

        void SetUp();
        bool Run();
        void TearDown();
        bool RunWithParameters(const ApplicationParameters& params);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // TraceMessageBus
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPrintf(const char* window, const char* message) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::EditorPythonConsoleNotifications
        void OnTraceMessage(AZStd::string_view message) override;
        void OnErrorMessage(AZStd::string_view message) override;
        void OnExceptionMessage(AZStd::string_view message) override;
        
        bool RunFileWithArgs(const ApplicationParameters& params);

        void StartPythonVM();
        void StopPythonVM();

    protected:
        bool m_showVerboseOutput = false;
        bool m_startedPython = false;
        int m_pythonExceptionCount = 0;
        int m_pythonErrorCount = 0;
    };
}
