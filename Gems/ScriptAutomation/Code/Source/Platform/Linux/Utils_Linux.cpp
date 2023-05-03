/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <sys/types.h>
#include <AzCore/PlatformIncl.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::Platform
{
    bool LaunchProgram(const AZStd::string& progPath, const AZStd::string& arguments)
    {
        AZ_Info("ScriptAutomation", "Attempting to launch \"%s %s\"", progPath.c_str(), arguments.c_str());

        // do this in the parent process for debugability.
        AZStd::string localProgPathCopy = progPath; // duplicate so we can have a non-const ptr to the data.
        AZStd::vector<AZStd::string> argumentTokens;
        auto SplitString = [&argumentTokens](AZStd::string_view token)
        {
            argumentTokens.emplace_back(token);
        };
        AZ::StringFunc::TokenizeVisitor(arguments, SplitString, " \t");

        AZStd::vector<char*> args;
        args.push_back(localProgPathCopy.data());
        AZ_Info("ScriptAutomation", "Attempting to launch app\"%s\"", localProgPathCopy.data());

        for (auto& argument: argumentTokens)
        {
            args.push_back(argument.data());
            AZ_Info("ScriptAutomation", "\targument:\"%s\"", argument.data());
        }
        args.push_back(static_cast<char*>(0));


        // Fork a process for execv to take over
        pid_t childPid = fork();

        if (childPid == 0)
        {
            // In child process
            execv(localProgPathCopy.c_str(), args.data());
            // execv never returns unless the launch failed.
            // The new program takes over the child process and will exit the child process when done.
            // This code should never be executed unless the execv call failed.
            AZ_Error(
                "LaunchProgram", false,
                "LaunchProgram: Unable to launch executable %s : errno = %s."
                "line tools.",
                localProgPathCopy.c_str(), strerror(errno));

            _exit(errno);
        }


        return true;
    }
}
