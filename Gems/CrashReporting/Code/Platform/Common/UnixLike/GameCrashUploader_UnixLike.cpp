/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CrashReporting/GameCrashUploader.h>
#include <CrashSupport.h>


namespace Lumberyard
{

    bool GameCrashUploader::CheckConfirmation(const crashpad::CrashReportDatabase::Report& report)
    {
        return true;
    }
}
