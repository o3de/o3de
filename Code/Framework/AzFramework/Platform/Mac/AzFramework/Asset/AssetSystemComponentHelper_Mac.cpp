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
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <sys/types.h>
#include <unistd.h>

namespace AzFramework::AssetSystem::Platform
{
    void AllowAssetProcessorToForeground()
    {}
    bool LaunchAssetProcessor(AZStd::string_view executableDirectory, AZStd::string_view appRoot,
        AZStd::string_view gameProjectName)
    {
        AZ::IO::FixedMaxPath assetProcessorPath{ executableDirectory };
        // In Mac the Editor and game is within a bundle, so the path to the sibling app
        // has to go up from the Contents/MacOS folder the binary is in
        assetProcessorPath /= "../../../AssetProcessor.app";
        assetProcessorPath = assetProcessorPath.LexicallyNormal();

        auto fullLaunchCommand = AZ::IO::FixedMaxPathString::format(R"(open -g "%s" --args --start-hidden)", assetProcessorPath.c_str());
        // Add the app-root to the launch command if not empty
        if (!appRoot.empty())
        {
            fullLaunchCommand += R"( --app-root=")";
            fullLaunchCommand += appRoot;
            fullLaunchCommand += '"';
        }

        // Add the active game project to the launch command if not empty 
        if (!gameProjectName.empty())
        {
            fullLaunchCommand += R"( --gameFolder=")";
            fullLaunchCommand += gameProjectName;
            fullLaunchCommand += '"';
        }

        return system(fullLaunchCommand.c_str()) == 0;
    }
}
