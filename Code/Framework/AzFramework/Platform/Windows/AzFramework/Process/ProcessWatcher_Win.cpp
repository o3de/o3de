/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzFramework/Process/ProcessCommon.h>

#include <iostream>

namespace AzFramework
{
    ProcessData::ProcessData()
    {
        Init(false);
    }

    void ProcessData::Init(bool stdCommunication)
    {
        ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
        ZeroMemory(&processInformation, sizeof(PROCESS_INFORMATION));
        if (stdCommunication)
        {
            startupInfo.dwFlags |= STARTF_USESTDHANDLES;
            inheritHandles = TRUE;
        }
        else
        {
            inheritHandles = FALSE;
        }

        jobHandle = nullptr;
        ZeroMemory(&jobCompletionPort, sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));
    }

    DWORD ProcessData::WaitForJobOrProcess(AZ::u32 waitTimeInMilliseconds) const
    {
        if (jobHandle)
        {
            // The completion query for JobObjects needs to be sliced in order to properly mimic the behaviour of WaitForSingleObject.  This is
            // because GetQueuedCompletionStatus can have several completion codes queued and the likelihood of the only event we care about 
            // being first in the queue is slim.  The choice of 5 attempts is arbitrary but seems to be enough to deplete the queue regardless
            // of whatever value waitTimeInMilliseconds is.
            const AZ::u32 totalWaitSteps = 5;
            const AZ::u32 slicedWaitTime = (waitTimeInMilliseconds / totalWaitSteps);

            DWORD completionCode;
            ULONG_PTR completionKey;
            LPOVERLAPPED overlapped;

            for (AZ::u32 waitStep = 0; waitStep < totalWaitSteps; ++waitStep)
            {
                if (GetQueuedCompletionStatus(jobCompletionPort.CompletionPort, &completionCode, &completionKey, &overlapped, slicedWaitTime))
                {
                    if (reinterpret_cast<HANDLE>(completionKey) == jobHandle && completionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO)
                    {
                        return WAIT_OBJECT_0;
                    }
                }
                else
                {
                    if (GetLastError() == ERROR_ABANDONED_WAIT_0)
                    {
                        return WAIT_ABANDONED;
                    }
                }
            }

            return WAIT_TIMEOUT;
        }
        else
        {
            return WaitForSingleObject(processInformation.hProcess, waitTimeInMilliseconds);
        }
    }

    bool ProcessLauncher::LaunchUnwatchedProcess(const ProcessLaunchInfo& processLaunchInfo)
    {
        ProcessData processData;
        processData.Init(false);
        return LaunchProcess(processLaunchInfo, processData);
    }

    bool ProcessLauncher::LaunchProcess(const ProcessLaunchInfo& processLaunchInfo, ProcessData& processData)
    {
        BOOL result = FALSE;

        // Windows API requires non-const char* command line string
        AZStd::wstring editableCommandLine;
        AZStd::wstring processExecutableString;
        AZStd::wstring workingDirectory;

        AZStd::string executableString = processLaunchInfo.m_processExecutableString;
        AZStd::string commandLine = processLaunchInfo.GetCommandLineParametersAsString();

        executableString = AZ::StringFunc::TrimWhiteSpace(executableString, true, true);
        commandLine = AZ::StringFunc::TrimWhiteSpace(commandLine, true, true);
        size_t pos;

        //create the executableString and commandLine from the supplied executableString, commandline, both, or fail
        if(executableString.length() && commandLine.length())
        {
            //user supplied both an executableString and a commandLine

            //see if the user supplied additional command line options in the executableString
            //if so remove them from the executableString and prepend them to the commandLine
            pos = AZ::StringFunc::Find(executableString, ".exe\"", 0, true);
            if(pos != AZStd::string::npos)
            {
                AZStd::string additionalCommandLine = executableString;
                AZ::StringFunc::RKeep(additionalCommandLine, pos + strlen(".exe\""));
                AZ::StringFunc::LKeep(executableString, pos + strlen(".exe\""));
                AZ::StringFunc::Prepend(commandLine, " ");
                AZ::StringFunc::Prepend(commandLine, additionalCommandLine.c_str());
            }
            else
            {
                pos = AZ::StringFunc::Find(executableString, ".exe", 0, true);
                if (pos != AZStd::string::npos)
                {
                    AZStd::string additionalCommandLine = executableString;
                    AZ::StringFunc::RKeep(additionalCommandLine, pos + strlen(".exe"));
                    AZ::StringFunc::LKeep(executableString, pos + strlen(".exe"));
                    AZ::StringFunc::Prepend(commandLine, " ");
                    AZ::StringFunc::Prepend(commandLine, additionalCommandLine.c_str());
                }
                else
                {
                    // the executable didnt specify ".exe" therefore we must assume the first space
                    pos = AZ::StringFunc::Find(executableString, " ");
                    if (pos != AZStd::string::npos)
                    {
                        // the executable string is everything to the left of the first space, anything to the right is commandLine
                        AZStd::string additionalCommandLine = executableString;
                        AZ::StringFunc::RKeep(additionalCommandLine, pos);
                        AZ::StringFunc::LKeep(executableString, pos);
                        AZ::StringFunc::Prepend(commandLine, " ");
                        AZ::StringFunc::Prepend(commandLine, additionalCommandLine.c_str());
                        AZ::StringFunc::Append(executableString, ".exe");
                    }
                    else
                    {
                        AZ::StringFunc::Append(executableString, ".exe");
                    }
                }
            }
        }
        else if(executableString.length())
        {
            //user supplied only an executableString
            pos = AZ::StringFunc::Find(executableString, ".exe\"", 0, true);
            if(pos != AZStd::string::npos)
            {
                //the executable string has .exe, so everything to the left is the process, anything to the right is commandLine
                commandLine = executableString;
                AZ::StringFunc::RKeep(commandLine, pos + strlen(".exe\""));
                AZ::StringFunc::LKeep(executableString, pos);
            }
            else
            {
                pos = AZ::StringFunc::Find(executableString, ".exe", 0, true);
                if(pos != AZStd::string::npos)
                {
                    //the executable string has .exe, so everything to the left is the process, anything to the right is commandLine
                    commandLine = executableString;
                    AZ::StringFunc::RKeep(commandLine, pos + strlen(".exe"));
                    AZ::StringFunc::LKeep(executableString, pos);
                }
                else
                {
                    // the executable didnt specify ".exe" therefore we must assume the first space
                    pos = AZ::StringFunc::Find(executableString, " ");
                    if (pos != AZStd::string::npos)
                    {
                        // the executable string is everything to the left of the first space, anything to the right is commandLine
                        commandLine = executableString;
                        AZ::StringFunc::RKeep(commandLine, pos);
                        AZ::StringFunc::LKeep(executableString, pos);
                        AZ::StringFunc::Append(executableString, ".exe");
                    }
                    else
                    {
                        AZ::StringFunc::Append(executableString, ".exe");
                    }
                }
            }
        }
        else if(commandLine.length())
        {
            //user supplied only a commandLine
            pos = AZ::StringFunc::Find(commandLine, ".exe\"");
            if(pos != AZStd::string::npos)
            {
                //the executable string has .exe, so everything to the left is the process, anything to the right is commandLine
                executableString = AZ::StringFunc::LKeep(commandLine, pos);
                commandLine = AZ::StringFunc::RKeep(commandLine, pos + strlen(".exe\""));
            }
            else
            {
                pos = AZ::StringFunc::Find(commandLine, ".exe");
                if(pos != AZStd::string::npos)
                {
                    //the executable string has .exe, so everything to the left is the process, anything to the right is commandLine
                    executableString = AZ::StringFunc::LKeep(commandLine, pos);
                    commandLine = AZ::StringFunc::RKeep(commandLine, pos + strlen(".exe"));
                }
                else
                {
                    // the executable didnt specify ".exe" therefore we must assume the first space
                    pos = AZ::StringFunc::Find(executableString, " ");
                    if (pos != AZStd::string::npos)
                    {
                        // the executable string is everything to the left of the first space, anything to the right is commandLine
                        commandLine = AZ::StringFunc::RKeep(executableString, pos);
                        executableString = AZ::StringFunc::LKeep(executableString, pos);
                        AZ::StringFunc::Append(executableString, ".exe");
                    }
                    else
                    {
                        AZ::StringFunc::Append(executableString, ".exe");
                    }
                }
            }
        }
        else
        {
            //use didnt supply either, fail
            return false;
        }

        //double quote any commandline args
        pos = 0;
        while(pos != AZStd::string::npos)
        {
            pos = AZ::StringFunc::Find(commandLine, "\"", pos);
            if(pos != AZStd::string::npos)
            {
                commandLine.insert(pos, "\\\"");
                pos += 3;
            }
        }

        //make sure the first commandline entry is the executableString
        pos = AZ::StringFunc::Find(commandLine, executableString.c_str());
        if(pos == AZStd::string::npos)
        {
            if (executableString.ends_with('\"'))
            {
                AZ::StringFunc::Prepend(commandLine, " ");
                AZ::StringFunc::Prepend(commandLine, executableString.c_str());
            }
            else
            {
                AZ::StringFunc::Prepend(commandLine, "\" ");
                AZ::StringFunc::Prepend(commandLine, executableString.c_str());
                AZ::StringFunc::Prepend(commandLine, "\"");
            }
            AZ::StringFunc::TrimWhiteSpace(commandLine, true, true);
        }

        AZStd::to_wstring(processExecutableString, executableString);
        AZStd::to_wstring(editableCommandLine, commandLine);
        AZStd::to_wstring(workingDirectory, processLaunchInfo.m_workingDirectory);

        AZStd::string environmentVariableBlock;
        if (processLaunchInfo.m_environmentVariables)
        {
            for (const auto& environmentVariable : *processLaunchInfo.m_environmentVariables)
            {
                environmentVariableBlock += environmentVariable;
                environmentVariableBlock.append(1, '\0');
            }
            environmentVariableBlock.append(processLaunchInfo.m_environmentVariables->size() ? 1 : 2, '\0'); // Double terminated, only need one if we ended with a null terminated string already
        }

        // Show or hide window
        processData.startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        processData.startupInfo.wShowWindow = processLaunchInfo.m_showWindow ? SW_SHOW : SW_HIDE;

        DWORD createFlags = 0;
        switch (processLaunchInfo.m_processPriority)
        {
        case PROCESSPRIORITY_BELOWNORMAL:
            createFlags |= BELOW_NORMAL_PRIORITY_CLASS;
            break;
        case PROCESSPRIORITY_IDLE:
            createFlags |= IDLE_PRIORITY_CLASS;
            break;
        }

        processData.jobHandle = CreateJobObject(nullptr, nullptr);
        if (processData.jobHandle)
        {
            processData.jobCompletionPort.CompletionKey = processData.jobHandle;
            processData.jobCompletionPort.CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);

            if (processData.jobCompletionPort.CompletionPort
                && SetInformationJobObject(processData.jobHandle, JobObjectAssociateCompletionPortInformation, &processData.jobCompletionPort, sizeof(processData.jobCompletionPort)))
            {
                createFlags |= CREATE_SUSPENDED;
            }
            else
            {
                CloseHandle(processData.jobCompletionPort.CompletionPort);
                ZeroMemory(&processData.jobCompletionPort, sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));

                CloseHandle(processData.jobHandle);
                processData.jobHandle = nullptr;
            }
        }

        // Create the child process.
        result = CreateProcessW(processExecutableString.size() ? processExecutableString.c_str() : NULL,
                editableCommandLine.size() ? editableCommandLine.data() : NULL, // command line
                NULL,  // process security attributes
                NULL,  // primary thread security attributes
                processData.inheritHandles,// handles might be inherited
                createFlags,     // creation flags
                environmentVariableBlock.size() ? environmentVariableBlock.data() : NULL, // environmentVariableBlock is a proper double null terminated block constructed above
                workingDirectory.empty() ? nullptr : workingDirectory.c_str(),  // use parent's current directory
                &processData.startupInfo, // STARTUPINFO pointer
                &processData.processInformation); // receives PROCESS_INFORMATION

        if (result != TRUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                processLaunchInfo.m_launchResult = PLR_MissingFile;
            }
        }
        else 
        {
            // attempt to attach the process to a job object so any additional child processes
            // that get spawned will be terminated correctly, if requested
            if (processData.jobHandle)
            {
                if (!AssignProcessToJobObject(processData.jobHandle, processData.processInformation.hProcess))
                {
                    CloseHandle(processData.jobCompletionPort.CompletionPort);
                    ZeroMemory(&processData.jobCompletionPort, sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));

                    CloseHandle(processData.jobHandle);
                    processData.jobHandle = nullptr;
                }

                ResumeThread(processData.processInformation.hThread);
            }
        }

        // Close inherited handles
        CloseHandle(processData.startupInfo.hStdInput);
        CloseHandle(processData.startupInfo.hStdOutput);
        CloseHandle(processData.startupInfo.hStdError);
        return result == TRUE;
    }


    ProcessWatcher* ProcessWatcher::LaunchProcess(const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, AzFramework::ProcessCommunicationType communicationType)
    {
        ProcessWatcher* pWatcher = new ProcessWatcher {};
        if (!pWatcher->SpawnProcess(processLaunchInfo, communicationType))
        {
            delete pWatcher;
            return nullptr;
        }
        return pWatcher;
    }


    ProcessWatcher::ProcessWatcher()
        : m_pCommunicator(nullptr)
    {
        m_pWatcherData = AZStd::make_unique<ProcessData>();
    }

    ProcessWatcher::~ProcessWatcher()
    {
        if (IsProcessRunning())
        {
            TerminateProcess(0);
        }

        delete m_pCommunicator;
        CloseHandle(m_pWatcherData->processInformation.hProcess);
        CloseHandle(m_pWatcherData->processInformation.hThread);
        if (m_pWatcherData->jobHandle)
        {
            CloseHandle(m_pWatcherData->jobCompletionPort.CompletionPort);
            CloseHandle(m_pWatcherData->jobHandle);
        }
    }

    StdProcessCommunicator* ProcessWatcher::CreateStdCommunicator()
    {
        return new StdInOutProcessCommunicator();
    }

    StdProcessCommunicatorForChildProcess* ProcessWatcher::CreateStdCommunicatorForChildProcess()
    {
        return new StdInOutProcessCommunicatorForChildProcess();
    }

    void ProcessWatcher::InitProcessData(bool stdCommunication)
    {
        m_pWatcherData->Init(stdCommunication);
    }

    namespace
    {
        // Returns true if process exited, false if still running
        bool CheckExitCode(const AzFramework::ProcessData* processData, AZ::u32* outExitCode = nullptr)
        {
            // Check exit code
            DWORD exitCode;
            BOOL result;

            if (processData->jobHandle)
            {
                JOBOBJECT_BASIC_ACCOUNTING_INFORMATION jobInfo;
                result = QueryInformationJobObject(processData->jobHandle,
                            JobObjectBasicAccountingInformation,
                            &jobInfo,
                            sizeof(jobInfo),
                            nullptr);

                if (!result)
                {
                    exitCode = 0;
                    AZ_Warning("ProcessWatcher", false, "QueryInformationJobObject failed (%d), assuming process either failed to launch or terminated unexpectedly\n", GetLastError());
                }
                else if (jobInfo.ActiveProcesses != 0)
                {
                    return false;
                }
            }

            result = GetExitCodeProcess(processData->processInformation.hProcess, &exitCode);
            if (!result)
            {
                exitCode = 0;
                AZ_TracePrintf("ProcessWatcher", "GetExitCodeProcess failed (%d), assuming process either failed to launch or terminated unexpectedly\n", GetLastError());
            }

            if (exitCode != STILL_ACTIVE)
            {
                if (outExitCode)
                {
                    *outExitCode = static_cast<AZ::u32>(exitCode);
                }
                return true;
            }
            return false;
        }
    }

    bool ProcessWatcher::IsProcessRunning(AZ::u32* outExitCode)
    {
        AZ_Assert(m_pWatcherData, "No watcher data");
        if (!m_pWatcherData)
        {
            return false;
        }

        if (CheckExitCode(m_pWatcherData.get(), outExitCode))
        {
            return false;
        }

        // Verify process is not signaled
        DWORD waitResult = m_pWatcherData->WaitForJobOrProcess(0);

        // if wait timed out, process still running.
        return waitResult == WAIT_TIMEOUT;
    }

    bool ProcessWatcher::WaitForProcessToExit(AZ::u32 waitTimeInSeconds, AZ::u32* outExitCode /*= nullptr*/)
    {
        if (CheckExitCode(m_pWatcherData.get()))
        {
            // Already exited
            return true;
        }

        // Verify process is not signaled
        DWORD waitResult = m_pWatcherData->WaitForJobOrProcess(waitTimeInSeconds * 1000);
        if ((outExitCode) && (waitResult != WAIT_TIMEOUT))
        {
            CheckExitCode(m_pWatcherData.get(), outExitCode);
        }

        // if wait timed out, process still running.
        return waitResult != WAIT_TIMEOUT;
    }

    void ProcessWatcher::TerminateProcess(AZ::u32 exitCode)
    {
        AZ_Assert(m_pWatcherData, "No watcher data");
        if (!m_pWatcherData)
        {
            return;
        }

        if (!IsProcessRunning())
        {
            return;
        }

        if (m_pWatcherData->jobHandle)
        {
            TerminateJobObject(m_pWatcherData->jobHandle, exitCode);
        }
        else
        {
            ::TerminateProcess(m_pWatcherData->processInformation.hProcess, exitCode);
        }
    }
} // namespace AzFramework
