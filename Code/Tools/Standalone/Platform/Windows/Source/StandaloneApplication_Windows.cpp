/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include <Source/StandaloneToolsApplication.h>

namespace StandaloneTools
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
