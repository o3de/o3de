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

#include <CrashSupport.h>
#include <algorithm>

#include <AzCore/PlatformIncl.h>
#include <time.h>

namespace CrashHandler
{
    void GetExecutablePathA(char* pathBuffer, int& bufferSize)
    {
        GetModuleFileNameA(nullptr, pathBuffer, bufferSize);
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