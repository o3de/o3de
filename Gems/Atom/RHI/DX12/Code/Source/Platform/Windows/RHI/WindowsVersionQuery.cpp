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
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/WindowsVersionQuery.h>

namespace AZ
{
    namespace DX12
    {
        namespace Platform
        {
            bool GetWindowsVersionFromSystemDLL(WindowsVersion* windowsVersion)
            {
                // We get the file version of one of the system DLLs to get the OS version.
                constexpr const char* dllName = "Kernel32.dll";
                VS_FIXEDFILEINFO* fileInfo = nullptr;
                DWORD handle;
                DWORD infoSize = GetFileVersionInfoSize(dllName, &handle);
                if (infoSize > 0)
                {
                    AZStd::vector<BYTE> versionData(infoSize);
                    if (GetFileVersionInfo(dllName, handle, infoSize, versionData.data()) != 0)
                    {
                        UINT len;
                        const char* subBlock = "\\";
                        if (VerQueryValue(versionData.data(), subBlock, reinterpret_cast<LPVOID*>(&fileInfo), &len) != 0)
                        {
                            windowsVersion->m_majorVersion = HIWORD(fileInfo->dwProductVersionMS);
                            windowsVersion->m_minorVersion = LOWORD(fileInfo->dwProductVersionMS);
                            windowsVersion->m_buildVersion = HIWORD(fileInfo->dwProductVersionLS);
                            return true;
                        }
                    }
                }
                return false;
            }

            bool GetWindowsVersion(WindowsVersion* windowsVersion)
            {
                static WindowsVersion s_cachedVersion;
                static bool s_isCached = false;
                if (s_isCached)
                {
                    *windowsVersion = s_cachedVersion;
                    return true;
                }
                
                bool readOk = GetWindowsVersionFromSystemDLL(windowsVersion);
                if (readOk)
                {
                    s_cachedVersion = *windowsVersion;
                    s_isCached = true;
                }
                return readOk;
            }
        }
    }
}
