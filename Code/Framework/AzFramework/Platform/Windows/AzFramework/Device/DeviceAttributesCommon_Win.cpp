/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Device/DeviceAttributeDeviceModel.h>
#include <AzFramework/Device/DeviceAttributeRAM.h>

namespace AzFramework
{
    DeviceAttributeDeviceModel::DeviceAttributeDeviceModel()
    {
        HKEY hKey = nullptr;
        LSTATUS returnCode = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(R"(HARDWARE\DESCRIPTION\System\BIOS)"), 0, KEY_READ, &hKey);
        if (returnCode == ERROR_SUCCESS)
        {
            wchar_t buf[255] = { 0 };
            DWORD dwBufSize = sizeof(buf);
            DWORD type = REG_SZ;
            if (::RegQueryValueEx(hKey, TEXT("SystemProductName"), NULL, &type, reinterpret_cast<LPBYTE>(buf), &dwBufSize) == ERROR_SUCCESS)
            {
                AZStd::to_string(m_value, buf);
            }
            ::RegCloseKey(hKey);
        }
    }

    DeviceAttributeRAM::DeviceAttributeRAM()
    {
        HMODULE kernel32Handle = ::GetModuleHandleW(L"Kernel32.dll");
        if (kernel32Handle)
        {
            using Func = BOOL(WINAPI*)(_Out_ LPMEMORYSTATUSEX lpMemStatus);
            auto globalMemoryStatusExFunc = reinterpret_cast<Func>(::GetProcAddress(kernel32Handle, "GlobalMemoryStatusEx"));
            if (globalMemoryStatusExFunc)
            {
                // OS RAM amount is returned in Bytes
                constexpr double bytesToGiB = 1024.0 * 1024.0 * 1024.0;
                MEMORYSTATUSEX memStats;
                memStats.dwLength = sizeof(memStats);
                if (globalMemoryStatusExFunc(&memStats))
                {
                    m_valueInGiB = aznumeric_cast<float>(static_cast<double>(memStats.ullTotalPhys) / bytesToGiB);
                }
            }

            // fall back to oldest available method 
            if (m_valueInGiB == 0)
            {
                // OS RAM amount is returned in Bytes
                constexpr double bytesToGiB = 1024.0 * 1024.0 * 1024.0;
                MEMORYSTATUS memStats;
                memStats.dwLength = sizeof(memStats);
                ::GlobalMemoryStatus(&memStats);
                m_valueInGiB = aznumeric_cast<float>(static_cast<double>(memStats.dwTotalPhys) / bytesToGiB);
            }
        }
    }
} // AzFramework

