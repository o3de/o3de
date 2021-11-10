/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CrashSupport.h>
#include <algorithm>

#include <AzCore/PlatformIncl.h>
#include <AzCore/Utils/Utils.h>
#include <time.h>

namespace CrashHandler
{
    void GetExecutablePath(char* pathBuffer, int& bufferSize)
    {
        AZ::Utils::GetExecutablePath(pathBuffer, bufferSize);
    }

    void GetExecutablePathW(wchar_t* pathBuffer, int& bufferSize)
    {
        GetModuleFileNameW(nullptr, pathBuffer, bufferSize);
    }

    void GetTimeInfo(tm& timeInfo)
    {
        time_t rawtime;

        time(&rawtime);
        localtime_s(&timeInfo, &rawtime);
    }
}
