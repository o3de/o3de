/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
