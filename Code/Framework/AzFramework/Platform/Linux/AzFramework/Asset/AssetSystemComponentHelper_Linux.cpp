/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    bool LaunchAssetProcessor(AZStd::string_view executableDirectory, AZStd::string_view engineRoot,
        AZStd::string_view projectPath)
    {
        AZ::IO::FixedMaxPath assetProcessorPath{ executableDirectory };
        assetProcessorPath /= "AssetProcessor";

        if (!AZ::IO::SystemFile::Exists(assetProcessorPath.c_str()))
        {
            // Check for existence of one under a "bin" directory, i.e. engineRoot is an SDK structure.
            assetProcessorPath =
                AZ::IO::FixedMaxPath{engineRoot} / "bin" / AZ_TRAIT_OS_PLATFORM_NAME / AZ_BUILD_CONFIGURATION_TYPE / "AssetProcessor";

            if (!AZ::IO::SystemFile::Exists(assetProcessorPath.c_str()))
            {
                return false;
            }
        }

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
                AZStd::array args {
                    assetProcessorPath.c_str(), assetProcessorPath.c_str(), "--start-hidden", 
                    static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), static_cast<const char*>(nullptr)
                };
                int optionalArgPos = 3;

                // Add the engine path to the launch command if not empty
                AZ::IO::FixedMaxPathString engineRootArg;
                if (!engineRoot.empty())
                {
                    engineRootArg = AZ::IO::FixedMaxPathString::format(R"(--engine-path="%.*s")", 
                        aznumeric_cast<int>(engineRoot.size()), engineRoot.data());
                    args[optionalArgPos++] = engineRootArg.data();
                }

                // Add the active project path to the launch command if not empty
                AZ::IO::FixedMaxPathString projectPathArg;
                if (!projectPath.empty())
                {
                    projectPathArg = AZ::IO::FixedMaxPathString::format(R"(--regset="/Amazon/AzCore/Bootstrap/project_path=%.*s")",
                        aznumeric_cast<int>(projectPath.size()), projectPath.data());
                    args[optionalArgPos++] = projectPathArg.data();
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
