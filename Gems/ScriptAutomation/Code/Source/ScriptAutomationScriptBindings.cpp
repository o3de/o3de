/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomationSystemComponent.h>

#include <ScriptAutomationScriptBindings.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>

#include <AzFramework/Components/ConsoleBus.h>

namespace ScriptAutomation
{
    namespace Bindings
    {
        void Print(const AZStd::string& message [[maybe_unused]])
        {
#ifndef _RELEASE //AZ_TracePrintf does nothing in release, ignore this call
            auto func = [message]()
            {
                AZ_TracePrintf("ScriptAutomation", "Script: %s\n", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
#endif
        }

        void Warning(const AZStd::string& message [[maybe_unused]])
        {
#ifndef _RELEASE //AZ_Warning does nothing in release, ignore this call
            auto func = [message]()
            {
                AZ_Warning("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
#endif
        }

        void Error(const AZStd::string& message [[maybe_unused]])
        {
#ifndef _RELEASE //AZ_Error does nothing in release, ignore this call
            auto func = [message]()
            {
                AZ_Error("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
#endif
        }

        void ExecuteConsoleCommand(const AZStd::string& command)
        {
            auto func = [command]()
            {
                AzFramework::ConsoleRequestBus::Broadcast(
                    &AzFramework::ConsoleRequests::ExecuteConsoleCommand, command.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void IdleFrames(int numFrames)
        {
            auto func = [numFrames]()
            {
                ScriptAutomationInterface::Get()->SetIdleFrames(numFrames);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void IdleSeconds(float numSeconds)
        {
            auto func = [numSeconds]()
            {
                ScriptAutomationInterface::Get()->SetIdleSeconds(numSeconds);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void LockFrameTime(float seconds)
        {
            auto func = [seconds]()
            {
                int milliseconds = static_cast<int>(seconds * 1000);
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand(AZStd::string::format("t_simulationTickDeltaOverride %d", milliseconds).c_str());

                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                automationComponent->SetFrameTimeIsLocked(true);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void UnlockFrameTime()
        {
            auto func = []()
            {
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand("t_simulationTickDeltaOverride 0");

                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                automationComponent->SetFrameTimeIsLocked(false);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void LoadLevel(const AZStd::string& levelPath)
        {
            auto func = [levelPath]()
            {
                AZStd::fixed_vector<AZStd::string_view, 2> loadLevelCmd;
                loadLevelCmd.push_back("LoadLevel");
                loadLevelCmd.push_back(levelPath);
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand(loadLevelCmd);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void StartBudgetTotalCapture()
        {
            auto startLogging = []()
            {
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand("ProfilerBudgetsStartCapture");
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(startLogging));
        }

        void StopBudgetTotalCapture()
        {
            auto stopLogging = []()
            {
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand("ProfilerBudgetsStopCapture");
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(stopLogging));
        }

        void WritePerfDataToCsvFile(const AZStd::string& filePath)
        {
            AZ::IO::FixedMaxPath resolvedPath;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, filePath.c_str());

            auto writePerfLogs = [resolvedPath]()
            {
                AZ::IO::SystemFile csvFile;
                csvFile.Open(resolvedPath.c_str(), AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH);

                const char* csvHeader = AZ::Statistics::NamedRunningStatistic::GetCsvHeader();
                csvFile.Write(csvHeader, strnlen_s(csvHeader, 1024));

                AZStd::vector<AZ::Statistics::NamedRunningStatistic*> stats;
                AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get()->GetAllStatisticsOfUnits(stats, "us");
                for (auto stat : stats)
                {
                    AZStd::string formattedStr = stat->GetCsvFormatted();
                    csvFile.Write(formattedStr.c_str(), formattedStr.length());
                    AZ_TracePrintf("ScriptAutomation", "%s", stat->GetFormatted().c_str());
                }

                csvFile.Close();
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(writePerfLogs));
        }

        float DegToRad(float degrees)
        {
            return AZ::DegToRad(degrees);
        }

        int GetRandomTestSeed()
        {
            auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
            auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
            return automationComponent->GetRandomTestSeed();
        }

        AZStd::string ResolvePath(const AZStd::string& path)
        {
            AZ::IO::FixedMaxPath resolvedPath;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, path.c_str());
            return resolvedPath.String();
        }

        AZStd::string NormalizePath(const AZStd::string& path)
        {
            AZStd::string normalizedPath = path;
            AZ::StringFunc::Path::Normalize(normalizedPath);
            return normalizedPath;
        }

        void RunScript(const AZStd::string& scriptFilePath)
        {
            // Unlike other Script_ callback functions, we process immediately instead of pushing onto the m_scriptOperations queue.
            // This function is special because running the script is what adds more commands onto the m_scriptOperations queue.
            ScriptAutomationInterface::Get()->ExecuteScript(scriptFilePath);
        }
    } // namespace Bindings

    void ReflectScriptBindings(AZ::BehaviorContext* behaviorContext)
    {
        AZ::MathReflect(behaviorContext);

        behaviorContext->Method("Print", &Bindings::Print);
        behaviorContext->Method("Warning", &Bindings::Warning);
        behaviorContext->Method("Error", &Bindings::Error);

        behaviorContext->Method("ExecuteConsoleCommand", &Bindings::ExecuteConsoleCommand);

        behaviorContext->Method("IdleFrames", &Bindings::IdleFrames);
        behaviorContext->Method("IdleSeconds", &Bindings::IdleSeconds);

        behaviorContext->Method("LockFrameTime", &Bindings::LockFrameTime);
        behaviorContext->Method("UnlockFrameTime", &Bindings::UnlockFrameTime);

        behaviorContext->Method("LoadLevel", &Bindings::LoadLevel);
        behaviorContext->Method("RunScript", &Bindings::RunScript);
        behaviorContext->Method("ResolvePath", &Bindings::ResolvePath);
        behaviorContext->Method("NormalizePath", &Bindings::NormalizePath);

        behaviorContext->Method("StartBudgetTotalCapture", &Bindings::StartBudgetTotalCapture);
        behaviorContext->Method("StopBudgetTotalCapture", &Bindings::StopBudgetTotalCapture);
        behaviorContext->Method("WritePerfDataToCsvFile", &Bindings::WritePerfDataToCsvFile);
    }
} // namespace ScriptAutomation
