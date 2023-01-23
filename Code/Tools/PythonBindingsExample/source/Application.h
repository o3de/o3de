/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(Application, AZ::SystemAllocator)
        Application(int* argc, char*** argv);
        ~Application();

        void SetUp();
        bool Run();
        void TearDown();
        bool RunWithParameters(const ApplicationParameters& params);

        inline void GetErrorCount(int& exceptionCount, int& errorCount)
        {
            exceptionCount = m_pythonExceptionCount;
            errorCount = m_pythonErrorCount;
        }

        inline void ResetErrorCount()
        {
            m_pythonExceptionCount = 0;
            m_pythonErrorCount = 0;
        }

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
