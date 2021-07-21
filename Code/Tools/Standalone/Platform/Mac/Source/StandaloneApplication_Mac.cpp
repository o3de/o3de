/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/StandaloneToolsApplication.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace StandaloneTools
{
    bool BaseApplication::LaunchDiscoveryService()
    {
        AZStd::string applicationFilePath;
        AzFramework::StringFunc::Path::Join(GetExecutableFolder(), "GridHub.app/Contents/MacOS/GridHub", applicationFilePath);
        if (AZ::IO::SystemFile::Exists(applicationFilePath.c_str()))
        {
            if (fork() == 0)
            {
                char* args[] = { const_cast<char*>(applicationFilePath.c_str()), nullptr };
                execv(applicationFilePath.c_str(), args);
            }
            return true;
        }
        AzFramework::StringFunc::Path::Join(GetExecutableFolder(), "GridHub", applicationFilePath);
        if (AZ::IO::SystemFile::Exists(applicationFilePath.c_str()))
        {
            if (fork() == 0)
            {
                char* args[] = { const_cast<char*>(applicationFilePath.c_str()), nullptr };
                execv(applicationFilePath.c_str(), args);
            }
            return true;
        }
        return false;
    }
}
