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

#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/tuple.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace AzFramework::AssetSystem::Platform
{
    void AllowAssetProcessorToForeground()
    {}
    bool LaunchAssetProcessor(AZStd::string_view executableDirectory, AZStd::string_view appRoot,
        AZStd::string_view gameProjectName)
    {
        pid_t firstChildPid = fork();
        if (firstChildPid == 0)
        {
            // redirect output to dev/null so it doesn't hijack an existing console window
            const char* devNull = AZ::IO::SystemFile::GetNullFilename();
            AZ::IO::FileDescriptorRedirector::Mode mode = AZ::IO::FileDescriptorRedirector::Mode::Create;

            AZ::IO::FileDescriptorRedirector stdoutRedirect(STDOUT_FILENO);
            stdoutRedirect.RedirectTo(devNull, mode);

            AZ::IO::FileDescriptorRedirector stderrRedirect(STDERR_FILENO);
            stderrRedirect.RedirectTo(devNull, mode);

            // detach the child from parent
            setsid();
            pid_t secondChildPid = fork();
            if (secondChildPid == 0)
            {
                AZ::IO::FixedMaxPath assetProcessorPath{ executableDirectory };
                assetProcessorPath /= "AssetProcessor";

                AZStd::array args {
                    assetProcessorPath.c_str(), assetProcessorPath.c_str(), "--start-hidden", 
                    static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), static_cast<const char*>(nullptr)
                };
                int optionalArgPos = 3;

                // Add the app-root to the launch command if not empty
                AZ::IO::FixedMaxPathString appRootArg;
                if (!appRoot.empty())
                {
                    appRootArg = AZ::IO::FixedMaxPathString::format(R"(--app-root="%.*s")", 
                        aznumeric_cast<int>(appRoot.size()), appRoot.data());
                    args[optionalArgPos++] = appRootArg.data();
                }

                // Add the active game project to the launch command if not empty
                AZ::IO::FixedMaxPathString projectArg;
                if (!gameProjectName.empty())
                {
                    projectArg = AZ::IO::FixedMaxPathString::format(R"(--regset="/Amazon/AzCore/Bootstrap/sys_game_folder=%.*s")",
                        aznumeric_cast<int>(gameProjectName.size()), gameProjectName.data());
                    args[optionalArgPos++] = projectArg.data();
                }

                AZStd::apply(execl, args);

                // exec* family of functions only exit on error
                AZ_Error("AssetSystemComponent", false, "Asset Processor failed with error: %s", strerror(errno));
                _exit(1);
            }

            stdoutRedirect.Reset();
            stderrRedirect.Reset();

            // exit the transient child with proper return code
            int ret = (secondChildPid < 0) ? 1 : 0;
            _exit(ret);
        }
        else if (firstChildPid > 0)
        {
            // wait for first child to exit to ensure the second child was started
            int status = 0; 
            pid_t ret = waitpid(firstChildPid, &status, 0);
            return (ret == firstChildPid) && (status == 0);
        }

        return false;
    }
}
