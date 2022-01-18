/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/tuple.h>

#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>

AZ_CVAR(bool, ap_tether_lifetime, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, a parent process that launches the AP will terminate the AP on exit");

namespace AzFramework::AssetSystem::Platform
{
    void AllowAssetProcessorToForeground()
    {}

    [[noreturn]] static void LaunchAssetProcessorDirectly(const AZ::IO::FixedMaxPath& assetProcessorPath, AZStd::string_view engineRoot, AZStd::string_view projectPath)
    {
        AZStd::fixed_vector<const char*, 5> args {
            assetProcessorPath.c_str(),
            "--start-hidden",
        };

        // Add the engine path to the launch command if not empty
        AZ::IO::FixedMaxPathString engineRootArg;
        if (!engineRoot.empty())
        {
            // No need to quote these paths, this code calls exec directly and
            // does not go through shell string interpolation
            engineRootArg = AZ::IO::FixedMaxPathString{"--engine-path="} + AZ::IO::FixedMaxPathString{engineRoot};
            args.push_back(engineRootArg.data());
        }

        // Add the active project path to the launch command if not empty
        AZ::IO::FixedMaxPathString projectPathArg;
        if (!projectPath.empty())
        {
            projectPathArg = AZ::IO::FixedMaxPathString{"--regset=/Amazon/AzCore/Bootstrap/project_path="} + AZ::IO::FixedMaxPathString{projectPath};
            args.push_back(projectPathArg.data());
        }

        // Make sure this is at the end
        args.push_back(nullptr); // argv itself needs to be null-terminated

        execv(args[0], const_cast<char**>(args.data()));

        // exec* family of functions only return on error
        fprintf(stderr, "Asset Processor failed with error: %s\n", strerror(errno));
        _exit(1);
    }

    static pid_t LaunchAssetProcessorDaemonized(const AZ::IO::FixedMaxPath& assetProcessorPath, AZStd::string_view engineRoot, AZStd::string_view projectPath)
    {
        // detach the child from parent
        setsid();
        const pid_t secondChildPid = fork();
        if (secondChildPid == 0)
        {
            LaunchAssetProcessorDirectly(assetProcessorPath, engineRoot, projectPath);
        }
        return secondChildPid;
    }

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

        const pid_t parentPid = getpid();
        const pid_t firstChildPid = fork();
        if (firstChildPid == 0)
        {
            // redirect output to dev/null so it doesn't hijack an existing console window
            const char* devNull = AZ::IO::SystemFile::GetNullFilename();
            AZ::IO::FileDescriptorRedirector::Mode mode = AZ::IO::FileDescriptorRedirector::Mode::Create;

            AZ::IO::FileDescriptorRedirector stdoutRedirect(STDOUT_FILENO);
            stdoutRedirect.RedirectTo(devNull, mode);

            AZ::IO::FileDescriptorRedirector stderrRedirect(STDERR_FILENO);
            stderrRedirect.RedirectTo(devNull, mode);

            if (ap_tether_lifetime)
            {
                prctl(PR_SET_PDEATHSIG, SIGTERM);
                if (getppid() != parentPid)
                {
                    _exit(1);
                }
                LaunchAssetProcessorDirectly(assetProcessorPath, engineRoot, projectPath);
            }
            else
            {
                const pid_t secondChildPid = LaunchAssetProcessorDaemonized(assetProcessorPath, engineRoot, projectPath);
                stdoutRedirect.Reset();
                stderrRedirect.Reset();

                // exit the transient child with proper return code
                int ret = (secondChildPid < 0) ? 1 : 0;
                _exit(ret);
            }

        }
        else if (firstChildPid > 0)
        {
            if (ap_tether_lifetime)
            {
                return true;
            }
            // wait for first child to exit to ensure the second child was started
            int status = 0; 
            pid_t ret = waitpid(firstChildPid, &status, 0);
            return (ret == firstChildPid) && (status == 0);
        }

        return false;
    }
} // namespace AzFramework::AssetSystem::Platform
