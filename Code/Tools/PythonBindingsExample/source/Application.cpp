/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/Application.h>
#include <source/ApplicationParameters.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/CommandLine/CommandRegistrationBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/Asset/AssetUtils.h>

#include <iostream>
#include <string>

namespace PythonBindingsExample
{
    //////////////////////////////////////////////////////////////////////////
    // Application

    Application::Application(int* argc, char*** argv)
        : AzToolsFramework::ToolsApplication(argc, argv)
    {
    }

    Application::~Application()
    {
        TearDown();
    }

    void Application::SetUp()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();

        // prepare the Python binding gem(s)
        Start(Descriptor());

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(context, "Application did not start; detected no serialize context");

        AZ_TracePrintf("Python Bindings", "Init() \n");
    }

    void Application::TearDown()
    {
        StopPythonVM();
        m_showVerboseOutput = false;
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        Stop();

    }

    void Application::StartPythonVM()
    {
        auto&& editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
        if (editorPythonEventsInterface)
        {
            // already started?
            if (m_startedPython == false)
            {
                m_startedPython = editorPythonEventsInterface->StartPython();
                AZ_Error("python_app", m_startedPython, "Python VM did not start.");
            }
        }
    }

    void Application::StopPythonVM()
    {
        auto&& editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
        if (editorPythonEventsInterface && m_startedPython)
        {
            editorPythonEventsInterface->StopPython();
            m_startedPython = false;
        }
        else if (m_showVerboseOutput)
        {
            AZ_Warning("python_app", false, "Python interface could not be stopped.");
        }
    }

    bool Application::Run()
    {
        ApplicationParameters params;
        if (params.Parse(GetCommandLine()))
        {
            return RunWithParameters(params);
        }
        return false;
    }

    bool Application::RunWithParameters(const ApplicationParameters& params)
    {
        using namespace AzFramework;
        using namespace AzToolsFramework;

        m_showVerboseOutput = params.m_verbose;

        auto&& editorPythonEventsInterface = AZ::Interface<EditorPythonEventsInterface>::Get();
        if (editorPythonEventsInterface)
        {
            try
            {
                StartPythonVM();
                if (!m_startedPython)
                {
                    AZ_Error("python_app", false, "Python VM did not start.");
                    return false;
                }
                
                if (params.m_pythonStatement.empty() == false)
                {
                    EditorPythonRunnerRequestBus::Broadcast(
                        &EditorPythonRunnerRequestBus::Events::ExecuteByString,
                        params.m_pythonStatement,
                        params.m_verbose);
                }

                if (params.m_pythonFilename.empty() == false)
                {
                    if (m_pythonExceptionCount == 0)
                    {
                        RunFileWithArgs(params);
                    }
                    else
                    {
                        AZ_Warning("python_app", false, "Did not execute script file since statement threw exceptions.");
                    }
                }

                const bool errorFree = (m_pythonExceptionCount == 0) && (m_pythonErrorCount == 0);
                if (errorFree && params.m_interactiveMode)
                {
                    AZSTD_PRINTF("Interactive mode enabled \n");
                    std::string statement;
                    bool executeStatement = false;
                    do
                    {
                        AZSTD_PRINTF("> ");
                        std::getline(std::cin, statement);
                        executeStatement = (statement.empty() == false);
                        if (executeStatement)
                        {
                            EditorPythonRunnerRequestBus::Broadcast(
                                &EditorPythonRunnerRequestBus::Events::ExecuteByString,
                                statement.c_str(),
                                params.m_verbose);
                        }
                    }
                    while (executeStatement);
                }

                if (errorFree == false)
                {
                    AZ_Warning("python_app", false, "Encountered %d exceptions and %d errors", m_pythonExceptionCount, m_pythonErrorCount);
                }
                return errorFree;
            }
            catch (const std::exception& e)
            {
                OnExceptionMessage(e.what());
            }
        }
        else
        {
            AZ_Error("python_app", false, "Python interface missing. "
                                          "This likely means that the project has not enabled the EditorPythonBindings gem.");
        }
        return false;
    }

    bool Application::RunFileWithArgs(const ApplicationParameters& params)
    {
        using namespace AzFramework;
        using namespace AzToolsFramework;
        
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO->Exists(params.m_pythonFilename.c_str()) == false)
        {
            AZ_Error("python_app", false, "Python file (%s) is missing.", params.m_pythonFilename.c_str());
            return false;
        }

        AZStd::vector<AZStd::string_view> pythonArgs;
        pythonArgs.reserve(params.m_pythonArgs.size());
        for(auto&& arg : params.m_pythonArgs)
        {
            pythonArgs.push_back(arg);
        }
        AZStd::transform(params.m_pythonArgs.begin(), params.m_pythonArgs.end(), pythonArgs.begin(), [](auto&& arg)
        {
            return arg.c_str();
        });

        EditorPythonRunnerRequestBus::Broadcast(
            &EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
            params.m_pythonFilename,
            pythonArgs);

        return m_pythonExceptionCount == 0;
    }

    bool Application::OnPreError(const char* window, const char* fileName, int line, [[maybe_unused]] const char* func, const char* message)
    {
        printf("\n");
        printf("[ERROR] - %s:\n", window);

        if (m_showVerboseOutput)
        {
            printf("(%s - Line %i)\n", fileName, line);
        }

        printf("%s \n", message);
        return true;
    }

    bool Application::OnPreWarning(const char* window, const char* fileName, int line, [[maybe_unused]] const char* func, const char* message)
    {
        // remove the warnings about the command line options from AZ::Console 
        if (AZ::StringFunc::Equal(window, "Az Console"))
        {
            return true;
        }

        printf("\n");
        printf("[WARN] - %s:\n", window);

        if (m_showVerboseOutput)
        {
            printf("(%s - Line %i)\n", fileName, line);
        }

        printf("%s \n", message);
        return true;
    }

    bool Application::OnPrintf([[maybe_unused]] const char* window, const char* message)
    {
        if (m_showVerboseOutput)
        {
            printf("%s\n", message);
            return true;
        }
        return !m_showVerboseOutput;
    }

    void Application::OnTraceMessage(AZStd::string_view message)
    {
        if (m_showVerboseOutput)
        {
            printf("(python) %.*s \n", aznumeric_cast<int>(message.size()), message.data());
        }
    }

    void Application::OnErrorMessage(AZStd::string_view message)
    {
        ++m_pythonErrorCount;
        printf("(python) [ERROR] %.*s \n", aznumeric_cast<int>(message.size()), message.data());
    }

    void Application::OnExceptionMessage(AZStd::string_view message)
    {
        ++m_pythonExceptionCount;
        printf("(python) [EXCEPTION] %.*s \n", aznumeric_cast<int>(message.size()), message.data());
    }
}
