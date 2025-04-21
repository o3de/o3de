/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/Process/ProcessWatcher.h>

namespace AZ::Platform
{
    bool LaunchProgram(const AZStd::string& progPath, const AZStd::string& arguments)
    {
        AZ_Info("ScriptAutomation", "Attempting to launch \"%s %s\"", progPath.c_str(), arguments.c_str());

        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        AZStd::vector<AZStd::string> launchCmd = { progPath };
        auto SplitString = [&launchCmd](AZStd::string_view token)
        {
            launchCmd.emplace_back(token);
        };
        AZ::StringFunc::TokenizeVisitor(arguments, SplitString, " \t");

        processLaunchInfo.m_commandlineParameters = AZStd::move(launchCmd);

        return AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
    }
}
