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

namespace Woodpecker
{
    bool BaseApplication::LaunchDiscoveryService()
    {
        WCHAR exeName[MAX_PATH] = { 0 };
        ::GetModuleFileName(::GetModuleHandle(nullptr), exeName, MAX_PATH);
        WCHAR drive[MAX_PATH] = { 0 };
        WCHAR dir[MAX_PATH] = { 0 };
        WCHAR discoveryServiceExe[MAX_PATH] = { 0 };
        WCHAR workingDir[MAX_PATH] = { 0 };

        _wsplitpath_s(exeName, drive, static_cast<size_t>(AZ_ARRAY_SIZE(drive)), dir, static_cast<size_t>(AZ_ARRAY_SIZE(dir)), nullptr, 0, nullptr, 0);
        _wmakepath_s(discoveryServiceExe, drive, dir, L"GridHub", L"exe");
        _wmakepath_s(workingDir, drive, dir, nullptr, nullptr);

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));

        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_MINIMIZE;

        PROCESS_INFORMATION pi;

        wchar_t commandLine[] = L"-fail_silently";
        return ::CreateProcessW(discoveryServiceExe, commandLine, nullptr, nullptr, FALSE, 0, nullptr, workingDir, &si, &pi) == TRUE;
    }
}
