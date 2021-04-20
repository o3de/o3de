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

#include <AzCore/PlatformIncl.h>
#include <CrashReporting/GameCrashUploader.h>
#include <CrashSupport.h>

namespace O3de
{
    void InstallCrashUploader(int& argc, char* argv[])
    {
        O3de::CrashUploader::SetCrashUploader(std::make_shared<O3de::GameCrashUploader>(argc, argv));
    }

    std::string GameCrashUploader::GetRootFolder()
    {
        std::string returnPath;
        ::CrashHandler::GetExecutableFolder(returnPath);

        return returnPath;
    }

    GameCrashUploader::GameCrashUploader(int& argCount, char** argv) :
        CrashUploader(argCount, argv)
    {

    }
}
