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

#include <AzFramework/Process/ProcessWatcher.h>
#include <AzFramework/Process/ProcessCommunicator.h>


namespace AzFramework
{

    StartupInfo::~StartupInfo()
    {

    }

    void StartupInfo::SetupHandlesForChildProcess()
    {

    }

    void StartupInfo::CloseAllHandles()
    {

    }

    bool ProcessLauncher::LaunchUnwatchedProcess([[maybe_unused]] const ProcessLaunchInfo& processLaunchInfo)
    {
        return false;
    }

    bool ProcessLauncher::LaunchProcess([[maybe_unused]] const ProcessLaunchInfo& processLaunchInfo, [[maybe_unused]] ProcessData& processData)
    {
        return false;
    }

    StdProcessCommunicator* ProcessWatcher::CreateStdCommunicator()
    {
        return new StdInOutProcessCommunicator();
    }

    StdProcessCommunicatorForChildProcess* ProcessWatcher::CreateStdCommunicatorForChildProcess()
    {
        return new StdInOutProcessCommunicatorForChildProcess();
    }

    ProcessWatcher* ProcessWatcher::LaunchProcess([[maybe_unused]] const ProcessLauncher::ProcessLaunchInfo& processLaunchInfo, [[maybe_unused]] ProcessCommunicationType communicationType)
    {
        return nullptr;
    }

    void ProcessWatcher::InitProcessData([[maybe_unused]] bool stdProcessData)
    {

    }

    ProcessWatcher::ProcessWatcher()
        : m_pCommunicator(nullptr)
    {

    }

    ProcessWatcher::~ProcessWatcher()
    {

    }

    bool ProcessWatcher::IsProcessRunning([[maybe_unused]] AZ::u32* outExitCode)
    {
        return false;
    }

    bool ProcessWatcher::WaitForProcessToExit([[maybe_unused]] AZ::u32 waitTimeInSeconds, [[maybe_unused]] AZ::u32* outExitCode /*= nullptr*/)
    {
        return true;
    }

    void ProcessWatcher::TerminateProcess([[maybe_unused]] AZ::u32 exitCode)
    {

    }
} //namespace AzFramework
