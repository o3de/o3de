/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Game Crashpad Hook - win

#include <CrashReporting/GameCrashHandler.h>

namespace CrashHandler
{
    const char* gameCrashHandlerPath = "GameCrash.Uploader.exe";

    const char* GameCrashHandler::GetCrashHandlerExecutableName() const
    {
        return gameCrashHandlerPath;
    }

}
