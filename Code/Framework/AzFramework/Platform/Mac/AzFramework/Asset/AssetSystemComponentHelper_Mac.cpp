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
        assetProcessorPath /= "../../../AssetProcessor.app";
        assetProcessorPath = assetProcessorPath.LexicallyNormal();

        if (!AZ::IO::SystemFile::Exists(assetProcessorPath.c_str()))
        {
            // Check for existence of one under a "bin" directory, i.e. engineRoot is an SDK structure.
            assetProcessorPath.Assign(engineRoot);
            assetProcessorPath /= "bin";
            #if defined(AZ_DEBUG_BUILD)
            assetProcessorPath /= "debug";
#elif defined(AZ_PROFILE_BUILD)
            assetProcessorPath /= "profile";
#else
            assetProcessorPath /= "release";
#endif
            assetProcessorPath /= "AssetProcessor.app";

            if (!AZ::IO::SystemFile::Exists(assetProcessorPath.c_str()))
            {
                return false;
            }
        }

        auto fullLaunchCommand = AZ::IO::FixedMaxPathString::format(R"(open -g "%s" --args --start-hidden)", assetProcessorPath.c_str());
        // Add the engine path to the launch command if not empty
        if (!engineRoot.empty())
        {
            fullLaunchCommand += R"( --engine-path=")";
            fullLaunchCommand += engineRoot;
            fullLaunchCommand += '"';
        }

        // Add the active project path to the launch command if not empty
        if (!projectPath.empty())
        {
            fullLaunchCommand += R"( --project-path=")";
            fullLaunchCommand += projectPath;
            fullLaunchCommand += '"';
        }

        return system(fullLaunchCommand.c_str()) == 0;
    }
}
