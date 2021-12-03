/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>
#include <AzCore/std/parallel/thread.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicator.h>

namespace AzFramework
{
    namespace ProcessLauncher
    {
        AZStd::string ProcessLaunchInfo::GetCommandLineParametersAsString() const
        {
            struct CommandLineParametersVisitor
            {
                AZStd::string operator()(const AZStd::string& commandLine) const
                {
                    return commandLine;
                }

                AZStd::string operator()(const AZStd::vector<AZStd::string>& commandLineArray) const
                {
                    AZStd::string commandLineResult;
#if AZ_TRAIT_AZFRAMEWORK_DOUBLE_ESCAPE_COMMANDLINE_QUOTES
                    // When re-constructing a command line from an argument list (on some platforms), if an argument
                    // is double-quoted, then the double-quotes must be escaped properly otherwise
                    // it will be absorbed by the native argument parser and possibly evaluated as
                    // multiple values for arguments
                    AZStd::string_view escapedDoubleQuote = R"("\")";

                    AZStd::vector<AZStd::string> preprocessed_command_array;
                    for (const auto & commandArg : commandLineArray )
                    {
                        AZStd::string replacedArg = commandArg;
                        auto replacePos = replacedArg.find_first_of('"');
                        while (replacePos != replacedArg.npos)
                        {
                            replacedArg.replace(replacePos, 1, escapedDoubleQuote.data(), escapedDoubleQuote.size());
                            replacePos += escapedDoubleQuote.size();
                            replacePos = replacedArg.find('"', replacePos);
                        }
                        preprocessed_command_array.emplace_back(replacedArg);

                    }
                    AzFramework::StringFunc::Join(commandLineResult, preprocessed_command_array.begin(), preprocessed_command_array.end(), " ");
#else
                    AzFramework::StringFunc::Join(commandLineResult, commandLineArray.begin(), commandLineArray.end(), " ");
#endif // AZ_TRAIT_AZFRAMEWORK_DOUBLE_ESCAPE_COMMANDLINE_QUOTES
                    return commandLineResult;
                }
            };

            return AZStd::visit(CommandLineParametersVisitor{}, m_commandlineParameters);
        }
    }

    bool ProcessWatcher::LaunchProcessAndRetrieveOutput(const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, ProcessCommunicationType communicationType, AzFramework::ProcessOutput& outProcessOutput)
    {
        // launch the process

        AZStd::scoped_ptr<ProcessWatcher> pWatcher(LaunchProcess(processLaunchInfo, communicationType));
        if (!pWatcher)
        {
            AZ_TracePrintf("Process Watcher", "ProcessWatcher::LaunchProcessAndRetrieveOutput: Unable to launch process '%s %s'\n", processLaunchInfo.m_processExecutableString.c_str(), processLaunchInfo.GetCommandLineParametersAsString().c_str());
            return false;
        }
        else
        {
            // get the communicator and ensure it is valid
            ProcessCommunicator* pCommunicator = pWatcher->GetCommunicator();
            if (!pCommunicator || !pCommunicator->IsValid())
            {
                AZ_TracePrintf("Process Watcher", "ProcessWatcher::LaunchProcessAndRetrieveOutput: No communicator for watcher's process (%s %s)!\n", processLaunchInfo.m_processExecutableString.c_str(), processLaunchInfo.GetCommandLineParametersAsString().c_str());
                return false;
            }
            else
            {
                pCommunicator->ReadIntoProcessOutput(outProcessOutput);
            }
        }
        return true;
    }


    bool ProcessWatcher::SpawnProcess(const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, ProcessCommunicationType communicationType)
    {
        InitProcessData(communicationType == COMMUNICATOR_TYPE_STDINOUT);

        if (communicationType == COMMUNICATOR_TYPE_STDINOUT)
        {
            StdProcessCommunicator* pStdCommunicator = CreateStdCommunicator();
            if (pStdCommunicator->CreatePipesForProcess(m_pWatcherData.get()))
            {
                m_pCommunicator = pStdCommunicator;
            }
            else
            {
                // Communicator failure, just clean it up
                delete pStdCommunicator;
            }
        }
        else if (communicationType == COMMUNICATOR_TYPE_NONE)
        {
            //Implemented, but don't do anything.
        }
        else
        {
            AZ_Assert(false, "communicationType %d not implemented", communicationType);
        }

        return ProcessLauncher::LaunchProcess(processLaunchInfo, *m_pWatcherData);
    }

    class ProcessCommunicator* ProcessWatcher::GetCommunicator()
    {
        return m_pCommunicator;
    }

    AZStd::shared_ptr<ProcessCommunicatorForChildProcess> ProcessWatcher::GetCommunicatorForChildProcess(ProcessCommunicationType communicationType)
    {
        if (communicationType == COMMUNICATOR_TYPE_STDINOUT)
        {
            StdProcessCommunicatorForChildProcess* communicator = CreateStdCommunicatorForChildProcess();
            if (!communicator->AttachToExistingPipes())
            {
                // Delete the communicator if attaching fails, it is useless
                delete communicator;
                communicator = nullptr;
            }
            return AZStd::shared_ptr<ProcessCommunicatorForChildProcess>{
                       communicator
            };
        }
        else if (communicationType == COMMUNICATOR_TYPE_NONE)
        {
            AZ_Assert(false, "No communicator for communicationType %d", communicationType);
        }
        else
        {
            AZ_Assert(false, "communicationType %d not implemented", communicationType);
        }
        return AZStd::shared_ptr<ProcessCommunicatorForChildProcess>{};
    }
} // AzFramework
