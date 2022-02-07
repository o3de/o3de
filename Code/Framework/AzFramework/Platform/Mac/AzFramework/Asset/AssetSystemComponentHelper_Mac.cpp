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

#include <AzFramework/Process/ProcessWatcher.h>

#include <sys/types.h>
#include <unistd.h>

namespace AzFramework::AssetSystem::Platform
{
    void AllowAssetProcessorToForeground()
    {}

    bool LaunchAssetProcessor(AZStd::string_view executableDirectory, AZStd::string_view engineRoot,
        AZStd::string_view projectPath)
    {
        AZ::IO::FixedMaxPath assetProcessorPath{ executableDirectory };
        // In Mac the Editor and game is within a bundle, so the path to the sibling app
        // has to go up from the Contents/MacOS folder the binary is in
        assetProcessorPath /= "../../../AssetProcessor.app/Contents/MacOS/AssetProcessor";
        assetProcessorPath = assetProcessorPath.LexicallyNormal();

        if (!AZ::IO::SystemFile::Exists(assetProcessorPath.c_str()))
        {
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                if (AZ::IO::FixedMaxPath installedBinariesPath;
                    settingsRegistry->Get(installedBinariesPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_InstalledBinaryFolder))
                {
                    // Check for existence of one under a "bin" directory, i.e. engineRoot is an SDK structure.
                    assetProcessorPath = AZ::IO::FixedMaxPath{ engineRoot } / installedBinariesPath / "AssetProcessor.app/Contents/MacOS/AssetProcessor";
                }
            }

            if (!AZ::IO::SystemFile::Exists(assetProcessorPath.c_str()))
            {
                return false;
            }
        }

        AZStd::string commandLineParams;
        // Add the engine path to the launch command if not empty
        if (!engineRoot.empty())
        {
            commandLineParams += AZStd::string::format("\"--engine-path=\"%s\"\"", engineRoot.data());
        }

        if (!projectPath.empty())
        {
            commandLineParams += AZStd::string::format(" \"--regset=/Amazon/AzCore/Bootstrap/project_path=\"%s\"\"", projectPath.data());
        }

        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
        processLaunchInfo.m_processExecutableString = AZStd::move(assetProcessorPath.Native());
        processLaunchInfo.m_commandlineParameters = commandLineParams;
        return AzFramework::ProcessLauncher::LaunchUnwatchedProcess(processLaunchInfo);
    }
}
