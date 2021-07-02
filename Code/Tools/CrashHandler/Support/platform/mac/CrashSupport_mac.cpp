/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CrashSupport.h>

#include <mach-o/dyld.h>  // Needed for _NSGetExecutablePath
#include <time.h>

namespace CrashHandler
{
    void GetExecutablePathA(char* pathBuffer, int& bufferSize)
    {
        unsigned int localBufferSize{static_cast<unsigned int>(bufferSize)};
        _NSGetExecutablePath(pathBuffer, &localBufferSize);
    }

    void GetExecutablePathW(wchar_t* pathBuffer, int& bufferSize)
    {
        // Windows only
    }

    void GetTimeInfo(tm& timeInfo)
    {
        time_t rawtime;

        time(&rawtime);
        localtime_r(&rawtime, &timeInfo);
    }
}
