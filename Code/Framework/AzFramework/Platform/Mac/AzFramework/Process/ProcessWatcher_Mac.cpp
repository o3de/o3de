/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicator.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <iostream>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/resource.h> // for iopolicy
#include <time.h>


namespace AzFramework
{
    namespace
    {
        /*! Checks to see if the child process specified by the id is done or not
         *
         * \param childProcessId - Id of the child process to check
         * \param outExitCode - any exit code the child process returned if it is not running
         * \return True if the process is still running, otherwise false
         */
        bool IsChildProcessDone(int childProcessId, AZ::u32* outExitCode = nullptr)
        {
            // Check exit code
            int exitCode = 0;
            int result = waitpid(childProcessId, &exitCode, WNOHANG);

            // result == 0 means child PID is still running, nothing to check
            if (result == -1)
            {
                AZ_TracePrintf("ProcessWatcher", "IsChildProcessDone could not determine child process status (waitpid errno %d). assuming process either failed to launch or terminated unexpectedly\n", errno);
                exitCode = 0;
            }
            else if (result == childProcessId)
            {
                // result == child PID indicates done
                int realExitCode = 0;
                if (WIFEXITED(exitCode))
                {
                    realExitCode = WEXITSTATUS(exitCode);
                }
                else if (WIFSIGNALED(exitCode))
                {
                    int termSig = WTERMSIG(exitCode);
                    if (termSig != 0)
                    {
                        realExitCode = termSig;
                    }

                    int coreDump = WCOREDUMP(exitCode);
                    if (coreDump != 0)
                    {
                        realExitCode = coreDump;
                    }
                }
                else if (WIFSTOPPED(exitCode))
                {
                    int stopSig = WSTOPSIG(exitCode);
                    realExitCode = stopSig;
                }
                exitCode = realExitCode;
            }

            if (outExitCode)
            {
                *outExitCode = exitCode;
            }

            return (result != 0);
        }

        inline bool IsIdChildProcess(pid_t processId)
        {
            return processId == 0;
        }

        /*! Executes a command in the child process after the fork operation has been executed.
         * This function will never return. If the execvp command fails this will call _exit with
         * the errno value as the return value since continuing execution after a execvp command
         * is invalid (it will be running the parent's code and in its address space and will 
         * cause many issues).
         *
         * \param commandAndArgs - Array of strings that has the command to execute in index 0 with any args for the command following. Last element must be a null pointer.
         * \param envionrmentVariables - Array of strings that contains environment variables that command should use. Last element must be a null pointer.
         * \param processLaunchInfo - struct containing information about luanching the command
         * \param startupInfo - struct containing information needed to startup the command
         */
        void ExecuteCommandAsChild(char** commandAndArgs, char** environmentVariables, const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, StartupInfo& startupInfo)
        {
            if (!processLaunchInfo.m_workingDirectory.empty())
            {
                int res = chdir(processLaunchInfo.m_workingDirectory.c_str());
                if (res != 0)
                {
                    std::cerr << strerror(errno) << std::endl;
                    AZ_TracePrintf("Process Watcher", "ProcessWatcher::LaunchProcessAndRetrieveOutput: Unable to change the launched process' directory to '%s'.", processLaunchInfo.m_workingDirectory.c_str());
                    // We *have* to _exit as we are the child process and simply
                    // returning at this point would mean we would start running
                    // the code from our parent process and that will just wreck
                    // havoc.
                    _exit(errno);
                }
            }

            switch (processLaunchInfo.m_processPriority)
            {
                case PROCESSPRIORITY_BELOWNORMAL:
                    nice(1);
                    // also reduce disk impact:
                    setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_UTILITY);
                    break;
                case PROCESSPRIORITY_IDLE:
                    nice(20);
                    // also reduce disk impact:
                    setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_THROTTLE);
                    break;
            }

            startupInfo.SetupHandlesForChildProcess();

            execve(commandAndArgs[0], commandAndArgs, environmentVariables);

            // If we get here then execve failed to run the requested program and
            // we have an error. In this case we need to exit the child process
            // to stop it from continuing to run as a clone of the parent
            AZ_TracePrintf("Process Watcher", "ProcessWatcher::LaunchProcessAndRetrieveOutput: Unable to launch process %s : errno = %s ", commandAndArgs[0], strerror(errno));
            std::cerr << strerror(errno) << std::endl;

            _exit(errno);
        }
    }

    StartupInfo::~StartupInfo()
    {
        CloseAllHandles();
    }

    void StartupInfo::SetupHandlesForChildProcess()
    {
        if (m_inputHandleForChild != STDIN_FILENO)
        {
            dup2(m_inputHandleForChild, STDIN_FILENO);
            close(m_inputHandleForChild);
        }

        if (m_outputHandleForChild != STDOUT_FILENO)
        {
            dup2(m_outputHandleForChild, STDOUT_FILENO);
            close(m_outputHandleForChild);
        }

        if (m_errorHandleForChild != STDERR_FILENO)
        {
            dup2(m_errorHandleForChild, STDERR_FILENO);
            close(m_errorHandleForChild);
        }
    }

    void StartupInfo::CloseAllHandles()
    {
        if (m_inputHandleForChild != -1)
        {
            close(m_inputHandleForChild);
            m_inputHandleForChild = -1;
        }

        if (m_outputHandleForChild != -1)
        {
            close(m_outputHandleForChild);
            m_outputHandleForChild = -1;
        }

        if (m_errorHandleForChild != -1)
        {
            close(m_errorHandleForChild);
            m_errorHandleForChild = -1;
        }
    }

    bool ProcessLauncher::LaunchUnwatchedProcess(const ProcessLaunchInfo& processLaunchInfo)
    {
        ProcessData processData = ProcessData();
        return LaunchProcess(processLaunchInfo, processData);
    }

    bool ProcessLauncher::LaunchProcess(const ProcessLaunchInfo& processLaunchInfo, ProcessData& processData)
    {
        bool result = false;

        // note that the convention here is that it uses windows-shell style escaping of combined args with spaces in it
        // (so surrounding with quotes like param="hello world")
        // this is so that the callers (which could be numerous) do not have to worry about this and sprinkle ifdefs
        // all over their code.
        // We'll convert this to UNIX style command line parameters by counting and eliminating quotes:
        
        AZStd::vector<AZStd::string> commandTokens;
        
        AZStd::string outputString;
        bool inQuotes = false;
        for (size_t pos = 0; pos < processLaunchInfo.m_commandlineParameters.size(); ++pos)
        {
            char currentChar = processLaunchInfo.m_commandlineParameters[pos];
            if (currentChar == '"')
            {
                // Allow quote literals to go through as quotes which do NOT alter our "in quotes" bool below
                // This is to conform with our PC parameter strings which will sometimes include path parameters which
                // Can have spaces and commas and need to be output as paramname="\"Some pa,ram\"" in order to capture both correctly
                if (outputString.length() && outputString.back() == '\\')
                {
                    outputString.back() = currentChar;
                }
                else
                {
                    inQuotes = !inQuotes;
                }
            }
            else if ((currentChar == ' ') && (!inQuotes))
            {
                // its a space outside of quotes, so it ends the current parameter
                commandTokens.push_back(outputString);
                outputString.clear();
            }
            else
            {
                // Its a normal character, or its a space inside quotes
                outputString.push_back(currentChar);
            }
        }
        
        if (!outputString.empty())
        {
            commandTokens.push_back(outputString);
            outputString.clear();
        }
        
        if (!processLaunchInfo.m_processExecutableString.empty())
        {
            commandTokens.insert(commandTokens.begin(), processLaunchInfo.m_processExecutableString);
        }

        AZStd::string commandNameWithPath = processLaunchInfo.m_workingDirectory + " " + commandTokens[0];
        if (AZ::IO::SystemFile::Exists(commandNameWithPath.c_str()))
        {
            return false;
        }

        // Because of the way execve is defined we need to copy the strings from
        // AZ::string (using c_str() returns a const char*) into a non-const char*

        // Need to add one more as exec requires the array's last element to be a null pointer
        char** commandAndArgs = new char*[commandTokens.size() + 1];
        for (int i = 0; i < commandTokens.size(); ++i)
        {
            const AZStd::string& token = commandTokens[i];
            commandAndArgs[i] = new char[token.size() + 1];
            commandAndArgs[i][0] = '\0';
            azstrcat(commandAndArgs[i], token.size(), token.c_str());
        }
        commandAndArgs[commandTokens.size()] = nullptr;

        char** environmentVariables = nullptr;
        int numEnvironmentVars = 0;
        if (processLaunchInfo.m_environmentVariables)
        {
            const int numEnvironmentVars = processLaunchInfo.m_environmentVariables->size();
            // Adding one more as exec expects the array to have a nullptr as the last element
            environmentVariables = new char*[numEnvironmentVars + 1];
            for (int i = 0; i < numEnvironmentVars; i++)
            {
                const AZStd::string& envVarString = processLaunchInfo.m_environmentVariables->at(i);
                environmentVariables[i] = new char[envVarString.size() + 1];
                environmentVariables[i][0] = '\0';
                azstrcat(environmentVariables[i], envVarString.size(), envVarString.c_str());
            }
            environmentVariables[numEnvironmentVars] = NULL;
        }

        pid_t child_pid = fork();
        if (IsIdChildProcess(child_pid))
        {
            ExecuteCommandAsChild(commandAndArgs, environmentVariables, processLaunchInfo, processData.m_startupInfo);
        }

        processData.m_childProcessId = child_pid;

        // Close these handles as they are only to be used by the child process
        processData.m_startupInfo.CloseAllHandles();

        if (processLaunchInfo.m_environmentVariables)
        {
            for (int i = 0; i < numEnvironmentVars; i++)
            {
                delete [] environmentVariables[i];
            }
            delete [] environmentVariables;
        }

        for (int i = 0; i < commandTokens.size(); i++)
        {
            delete [] commandAndArgs[i];
        }
        delete [] commandAndArgs;

        // If an error occurs, exit the application.
        return child_pid >= 0;
    }

    StdProcessCommunicator* ProcessWatcher::CreateStdCommunicator()
    {
        return new StdInOutProcessCommunicator();
    }

    StdProcessCommunicatorForChildProcess* ProcessWatcher::CreateStdCommunicatorForChildProcess()
    {
        return new StdInOutProcessCommunicatorForChildProcess();
    }

    ProcessWatcher* ProcessWatcher::LaunchProcess(const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, ProcessCommunicationType communicationType)
    {
        ProcessWatcher* pWatcher = new ProcessWatcher {};
        if (!pWatcher->SpawnProcess(processLaunchInfo, communicationType))
        {
            delete pWatcher;
            return nullptr;
        }
        return pWatcher;
    }

    void ProcessWatcher::InitProcessData(bool stdProcessData)
    {
        /** Nothing to do for this on macOS */
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
    }

    bool ProcessWatcher::IsProcessRunning(AZ::u32* outExitCode)
    {
        AZ_Assert(m_pWatcherData, "No watcher data");
        if (!m_pWatcherData || m_pWatcherData->m_childProcessIsDone)
        {
            return false;
        }

        m_pWatcherData->m_childProcessIsDone = IsChildProcessDone(m_pWatcherData->m_childProcessId, outExitCode);
        return !m_pWatcherData->m_childProcessIsDone;
    }

    bool ProcessWatcher::WaitForProcessToExit(AZ::u32 waitTimeInSeconds, AZ::u32* outExitCode /*= nullptr*/)
    {
        AZ_UNUSED(outExitCode);

        if (IsChildProcessDone(m_pWatcherData->m_childProcessId))
        {
            // Already exited
            return true;
        }

        bool isProcessDone = false;
        time_t startTime = time(0);
        time_t currentTime = startTime;
        AZ_Assert(currentTime != -1, "time(0) returned an invalid time");
        while (((currentTime - startTime) < waitTimeInSeconds) && !isProcessDone)
        {
            usleep(100);
            int wait_status = 0;
            int result = waitpid(m_pWatcherData->m_childProcessId, &wait_status, WNOHANG);
            if (result == m_pWatcherData->m_childProcessId)
            {
                isProcessDone = true;
                m_pWatcherData->m_childProcessIsDone = true;
                if (outExitCode)
                {
                    *outExitCode = static_cast<AZ::u32>(WEXITSTATUS(wait_status));
                }
            }
            currentTime = time(0);
        }
        //returns false if process is still running after time
        return isProcessDone;
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

        kill(m_pWatcherData->m_childProcessId, SIGKILL);
        waitpid(m_pWatcherData->m_childProcessId, NULL, 0);
    }
} //namespace AzFramework

