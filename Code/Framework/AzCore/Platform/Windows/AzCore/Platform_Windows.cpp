/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Platform.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/Math/Sha1.h>

#include <tchar.h>

namespace AZ
{
    namespace Platform
    {
        ProcessId GetCurrentProcessId()
        {
            return static_cast<ProcessId>(::GetCurrentProcessId());
        }

        MachineId GetLocalMachineId()
        {
            if (s_machineId == 0)
            {
                HKEY key;
                long ret;
                // Query the machine guid generated at install time by windows, which is generated from
                // hardware signatures. This guid is not enough, because images (AWS) may duplicate this,
                // so include the hostname/username as well
                TCHAR machineInfo[MAX_COMPUTERNAME_LENGTH + 1024] = { 0 };
                ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_QUERY_VALUE, &key);
                if (ret == ERROR_SUCCESS)
                {
                    DWORD dataType = REG_SZ;
                    DWORD dataSize = sizeof(machineInfo);
                    ret = RegQueryValueEx(key, "MachineGuid", 0, &dataType, (LPBYTE)machineInfo, &dataSize);
                    RegCloseKey(key);
                }
                else
                {
                    AZ_Error("System", false, "Failed to open HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\MachineGuid!")
                }

                TCHAR* hostname = machineInfo + _tcslen(machineInfo);
                // salt the guid time with ComputerName/UserName
                DWORD  bufCharCount = DWORD(sizeof(machineInfo) - (hostname - machineInfo));
                if (!::GetComputerName(hostname, &bufCharCount))
                {
                    AZ_Error("System", false, "GetComputerName filed with code %d", GetLastError());
                }

                TCHAR* username = hostname + _tcslen(hostname);
                bufCharCount = DWORD(sizeof(machineInfo) - (username - machineInfo));
                if( !GetUserName( username, &bufCharCount ) ) 
                {
                    AZ_Error("System",false,"GetUserName filed with code %d",GetLastError());
                }

                Sha1 hash;
                AZ::u32 digest[5] = { 0 };
                hash.ProcessBytes(machineInfo, _tcslen(machineInfo) * sizeof(TCHAR));
                hash.GetDigest(digest);
                s_machineId = digest[0];
                if (s_machineId == 0)
                {
                    s_machineId = 1;
                    AZ_Warning("System", false, "0 machine ID is reserved!");
                }
            }
            return s_machineId;
        }

    } // namespace Platform
}
