/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
