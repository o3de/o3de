/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <mutex>

namespace AzFramework
{
    namespace ProcessLauncher
    {
        /*! Checks to see if the child process specified by the id is done or not
         *
         * \param childProcessId - Id of the child process to check
         * \param outExitCode - any exit code the child process returned if it is not running
         * \return True if the process is still running, otherwise false
         */
        bool IsChildProcessDone(int childProcessId, AZ::u32* outExitCode = nullptr)
        {
            int childState;
            const pid_t rc_pid = waitpid(static_cast<pid_t>(childProcessId), &childState, WNOHANG);
            if (rc_pid > 0)
            {
                if (WIFEXITED(childState)) // exited
                {
                    const int exitStatus = WEXITSTATUS(childState);
                    if (exitStatus != 0)
                    {
                        AZ_TracePrintf("ProcessWatcher", "Child process id %d terminated prematurely (exit status %d)\n", childProcessId, exitStatus);
                    }
                    if (outExitCode)
                    {
                        *outExitCode = exitStatus;
                    }
                    return true;
                } 
                else if(WIFSIGNALED(childState)) // signaled
                {
                    const int signalStatus = WTERMSIG(childState);
                    AZ_TracePrintf("ProcessWatcher", "Child process id %d terminated prematurely (signal %d)\n", childProcessId, signalStatus);
                    if (outExitCode)
                    {
                        *outExitCode = signalStatus;
                    }
                    return true;
                }
                else
                {
                    return false; // still running
                }
            } 
            else if (rc_pid < 0) 
            {
                if (errno == ECHILD) 
                {
                    AZ_TracePrintf("ProcessWatcher", "Child process id %d does not exist\n", childProcessId);
                    return true;
                } 
                else 
                {
                    AZ_Assert(false, "Bad argument passed to waitpid\n");
                }
            }
            return false;
        }

        inline bool IsIdChildProcess(pid_t processId)
        {
            return processId == 0;
        }

        /*! Executes a command in the child process after the fork operation
         * has been executed. This function will never return. If the execvpe
         * command fails this will call _exit since continuing execution after
         * a execvpe command is invalid (it will be running the parent's code
         * and in its address space and will cause many issues).
         *
         * This function runs after a `fork()` call. `fork()` creates a copy of
         * the current process, including the current state of the process's
         * memory, at the time the call is made. However, it only creates a
         * copy of the one thread that called `fork()`. This means that if any
         * mutexes are locked by other threads at the time of `fork()`, those
         * mutexes will remain locked in the child process, with no way to
         * unlock them. So this function needs to ensure that it does as little
         * work as possible.
         *
         * \param commandAndArgs - Array of strings that has the command to execute in index 0 with any args for the command following. Last element must be a null pointer.
         * \param environmentVariables - Array of strings that contains environment variables that command should use. Last element must be a null pointer.
         * \param processLaunchInfo - struct containing information about luanching the command
         * \param startupInfo - struct containing information needed to startup the command
         * \param errorPipe - a pipe file descriptor used to communicate a failed execvpe call's error code to the parent process
         */
        [[noreturn]] static void ExecuteCommandAsChild(char** commandAndArgs, char** environmentVariables, const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, StartupInfo& startupInfo, const AZStd::array<int, 2>& errorPipe)
        {
            close(errorPipe[0]);

            if (!processLaunchInfo.m_workingDirectory.empty())
            {
                int res = chdir(processLaunchInfo.m_workingDirectory.c_str());
                if (res != 0)
                {
                    write(errorPipe[1], &errno, sizeof(int));
                    // We *have* to _exit as we are the child process and simply
                    // returning at this point would mean we would start running
                    // the code from our parent process and that will just wreck
                    // havoc.
                    _exit(0);
                }
            }

            switch (processLaunchInfo.m_processPriority)
            {
                case PROCESSPRIORITY_BELOWNORMAL:
                    nice(1);
                    // also reduce disk impact:
                    // setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_UTILITY);
                    break;
                case PROCESSPRIORITY_IDLE:
                    nice(20);
                    // also reduce disk impact:
                    // setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_THROTTLE);
                    break;
            }

            startupInfo.SetupHandlesForChildProcess();

            execvpe(commandAndArgs[0], commandAndArgs, environmentVariables);
            const int errval = errno;

            // If we get here then execvpe failed to run the requested program and
            // we have an error. In this case we need to exit the child process
            // to stop it from continuing to run as a clone of the parent.
            // Communicate the error code back to the parent via a pipe for the
            // parent to read.
            write(errorPipe[1], &errval, sizeof(errval));

            _exit(0);
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
        // note that the convention here is that it uses windows-shell style escaping of combined args with spaces in it
        // (so surrounding with quotes like param="hello world")
        // this is so that the callers (which could be numerous) do not have to worry about this and sprinkle ifdefs
        // all over their code.
        // We'll convert this to UNIX style command line parameters by counting and eliminating quotes:
        
        AZStd::vector<AZStd::string> commandTokens;
        
        AZStd::string outputString;
        bool inQuotes = false;
        for (const char currentChar : processLaunchInfo.m_commandlineParameters)
        {
            if (currentChar == '"')
            {
                inQuotes = !inQuotes;
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

        // Because of the way execvpe is defined we need to copy the strings from
        // AZ::string (using c_str() returns a const char*) into a non-const char*

        // Need to add one more as execvpe requires the array's last element to be a null pointer
        char** commandAndArgs = new char*[commandTokens.size() + 1];
        for (int i = 0; i < commandTokens.size(); ++i)
        {
            const AZStd::string& token = commandTokens[i];
            commandAndArgs[i] = new char[token.size() + 1];
            commandAndArgs[i][0] = '\0';
            azstrcat(commandAndArgs[i], token.size(), token.c_str());
        }
        commandAndArgs[commandTokens.size()] = nullptr;

        AZStd::vector<AZStd::unique_ptr<char[]>> environmentVariablesManaged;
        AZStd::vector<char*> environmentVariablesVector;
        char** environmentVariables = nullptr;
        if (processLaunchInfo.m_environmentVariables)
        {
            for (const auto& envVarString : *processLaunchInfo.m_environmentVariables)
            {
                auto& environmentVariable = environmentVariablesManaged.emplace_back(AZStd::make_unique<char[]>(envVarString.size() + 1));
                environmentVariable[0] = '\0';
                azstrcat(environmentVariable.get(), envVarString.size() + 1, envVarString.c_str());
                environmentVariablesVector.emplace_back(environmentVariable.get());
            }
            // Adding one more as execvpe expects the array to have a nullptr as the last element
            environmentVariablesVector.emplace_back(nullptr);
            environmentVariables = environmentVariablesVector.data();
        }
        else
        {
            // If no environment variables were specified, then use the current process's environment variables
            // and pass it along for the execute .
            extern char **environ;              // Defined in unistd.h
            environmentVariables = ::environ;
            AZ_Assert(environmentVariables, "Environment variables for current process not available\n");
        }

        // Set up a pipe to communicate the error code from the subprocess's execvpe call
        AZStd::array<int, 2> childErrorPipeFds{};
        pipe(childErrorPipeFds.data());

        // This configures the write end of the pipe to close on calls to `exec`
        fcntl(childErrorPipeFds[1], F_SETFD, fcntl(childErrorPipeFds[1], F_GETFD) | FD_CLOEXEC);

        pid_t child_pid = fork();
        if (IsIdChildProcess(child_pid))
        {
            ExecuteCommandAsChild(commandAndArgs, environmentVariables, processLaunchInfo, processData.m_startupInfo, childErrorPipeFds);
        }

        // Close these handles as they are only to be used by the child process
        processData.m_startupInfo.CloseAllHandles();
        close(childErrorPipeFds[1]);

        {
            int errorCodeFromChild = 0;
            int count = 0;
            // Read from the error pipe.
            // * If the child's call to execvpe succeeded, then the pipe will
            // be closed due to setting FD_CLOEXEC on the write end of the
            // pipe. `read()` will return 0.
            // * If the child's call to execvpe failed, the child will have
            // written the error code to the pipe. `read()` will return >0, and
            // the data to be read is the error code from execvpe.
            while ((count = read(childErrorPipeFds[0], &errorCodeFromChild, sizeof(errorCodeFromChild))) == -1)
            {
                if (errno != EAGAIN && errno != EINTR)
                {
                    break;
                }
            }
            if (count)
            {
                AZ_TracePrintf("Process Watcher", "ProcessLauncher::LaunchProcess: Unable to launch process %s : errno = %s\n", commandAndArgs[0], strerror(errorCodeFromChild));
                processData.m_childProcessIsDone = true;
                child_pid = -1;
            }
        }
        close(childErrorPipeFds[0]);

        processData.m_childProcessId = child_pid;

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

        m_pWatcherData->m_childProcessIsDone = ProcessLauncher::IsChildProcessDone(m_pWatcherData->m_childProcessId, outExitCode);
        return !m_pWatcherData->m_childProcessIsDone;
    }

    bool ProcessWatcher::WaitForProcessToExit(AZ::u32 waitTimeInSeconds, AZ::u32* outExitCode /*= nullptr*/)
    {
        if (ProcessLauncher::IsChildProcessDone(m_pWatcherData->m_childProcessId, outExitCode))
        {
            // Already exited
            m_pWatcherData->m_childProcessIsDone = true;
            return true;
        }

        bool isProcessDone = false;
        time_t startTime = time(nullptr);
        time_t currentTime = startTime;
        AZ_Assert(currentTime != -1, "time(0) returned an invalid time");
        while (((currentTime - startTime) < waitTimeInSeconds) && !isProcessDone)
        {
            usleep(100);
            if (ProcessLauncher::IsChildProcessDone(m_pWatcherData->m_childProcessId, outExitCode))
            {
                isProcessDone = true;
                m_pWatcherData->m_childProcessIsDone = true;
                break;
            }
            currentTime = time(nullptr);
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
    }
} //namespace AzFramework
