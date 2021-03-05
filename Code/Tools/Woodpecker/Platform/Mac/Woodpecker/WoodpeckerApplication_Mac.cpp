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

#include "Woodpecker_precompiled.h"
#include <Woodpecker/WoodpeckerApplication.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace Woodpecker
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
