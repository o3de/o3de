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
            bool result = true;

            // Fork a process to run Beyond Compare app
            pid_t childPid = fork();

            if (childPid == 0)
            {
                AZStd::vector<AZstd::string> argumentTokens;
                auto SplitString = [&argumentTokens](AZStd::string_view token)
                {
                    argumentTokens.emplace_back(token);
                };
                AZ::StringFunc::TokenizeVisitor(arguments, SplitString, " \t");
                // In child process
                AZStd::vector<char*> args;
                args.push_back(progPath.c_str());
                for (const auto& argument: argumentTokens)
                {
                    args.push_back(argument.c_str());
                }
                args.push_back(static_cast<char*>(0));
                execv(progPath.c_str(), args.data());

                AZ_Error(
                    "LaunchProgram", false,
                    "LaunchProgram: Unable to launch executable %s : errno = %s."
                    "line tools.",
                    progPath.c_str(), strerror(errno));

                _exit(errno);
            }
            else
            {
                result = false;
            }

            return result;
    }
}
