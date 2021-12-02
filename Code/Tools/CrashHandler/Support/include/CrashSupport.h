/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Crash Handler shared support

#pragma once

#include <AzCore/PlatformDef.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>

#include <algorithm>
#include <string>

#define CRASH_HANDLER_MAX_PATH_LEN 1024

#define _MAKE_DEFINE_STRING(x) #x
#define MAKE_DEFINE_STRING(x) _MAKE_DEFINE_STRING(x)

namespace CrashHandler
{
    std::string GetTimeString();
    void GetTimeInfo(tm& curTime);

    inline void GetExecutablePath(std::string& returnPath)
    {
        char currentFileName[CRASH_HANDLER_MAX_PATH_LEN] = { 0 };
        AZ::Utils::GetExecutablePath(currentFileName, CRASH_HANDLER_MAX_PATH_LEN);

        returnPath = currentFileName;
        std::replace(returnPath.begin(), returnPath.end(), '\\', '/');
    }

    inline void GetExecutablePath(std::wstring& returnPathW)
    {
        std::string returnPath;
        GetExecutablePath(returnPath);
        wchar_t currentFileNameW[CRASH_HANDLER_MAX_PATH_LEN] = { 0 };
        AZStd::to_wstring(currentFileNameW, CRASH_HANDLER_MAX_PATH_LEN, { returnPath.c_str(), returnPath.size() });
        returnPathW = currentFileNameW;
    }

    template<typename T>
    inline void GetExecutableFolder(T& returnPath)
    {
        GetExecutablePath(returnPath);

        auto lastPos = returnPath.find_last_of('/');
        if (lastPos != T::npos)
        {
            returnPath = returnPath.substr(0, lastPos + 1);
        }
    }

    template<typename T>
    inline void GetExecutableBaseName(T& returnPath)
    {
        GetExecutablePath(returnPath);

        auto lastPos = returnPath.find_last_of('/');
        if (lastPos != T::npos)
        {
            returnPath = returnPath.substr(lastPos + 1, returnPath.length());
        }

        auto extPos = returnPath.find_last_of('.');
        if (extPos != T::npos)
        {
            returnPath = returnPath.substr(0, extPos);
        }
    }
}
