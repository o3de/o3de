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
    bool ProcessWatcher::LaunchProcessAndRetrieveOutput(const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, ProcessCommunicationType communicationType, AzFramework::ProcessOutput& outProcessOutput)
    {
        // launch the process

        AZStd::scoped_ptr<ProcessWatcher> pWatcher(LaunchProcess(processLaunchInfo, communicationType));
        if (!pWatcher)
        {
            AZ_TracePrintf("Process Watcher", "ProcessWatcher::LaunchProcessAndRetrieveOutput: Unable to launch process '%s %s'\n", processLaunchInfo.m_processExecutableString.c_str(), processLaunchInfo.m_commandlineParameters.c_str());
            return false;
        }
        else
        {
            // get the communicator and ensure it is valid
            ProcessCommunicator* pCommunicator = pWatcher->GetCommunicator();
            if (!pCommunicator || !pCommunicator->IsValid())
            {
                AZ_TracePrintf("Process Watcher", "ProcessWatcher::LaunchProcessAndRetrieveOutput: No communicator for watcher's process (%s %s)!\n", processLaunchInfo.m_processExecutableString.c_str(), processLaunchInfo.m_commandlineParameters.c_str());
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
