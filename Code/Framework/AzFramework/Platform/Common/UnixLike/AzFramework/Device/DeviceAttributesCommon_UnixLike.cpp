/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
*
 */

#include <sys/utsname.h>
#include <AzFramework/Device/DeviceAttributeDeviceModel.h>
#include <AzFramework/Device/DeviceAttributeRAM.h>
#include <AzCore/std/utility/charconv.h>

namespace AzFramework
{
    DeviceAttributeDeviceModel::DeviceAttributeDeviceModel()
    {
        utsname systemInfo;
        if (uname(&systemInfo) != -1)
        {
            m_value = systemInfo.machine;
        }
    }

    DeviceAttributeRAM::DeviceAttributeRAM()
    {
        FILE* f;
        azfopen(&f, "/proc/meminfo", "r");
        if (f)
        {
            char buffer[256] = { 0 };
            AZ::u64 memTotalKiB = 0;
            constexpr AZStd::string_view MemTotalKey = "MemTotal:";
            while (fgets(buffer, sizeof(buffer), f))
            {
                const AZStd::string_view bufferView(buffer);
                if(size_t offset = bufferView.find(MemTotalKey); offset != AZStd::string_view::npos)
                {
                    auto result = AZStd::from_chars(bufferView.begin() + MemTotalKey.size(), bufferView.end(), memTotalKiB);
                    if(result.ec != AZStd::errc{})
                    {
                        constexpr double KiBtoGiB = 1024.f * 1024.f;
                        m_valueInGiB = aznumeric_cast<float>(static_cast<double>(memTotalKiB) / KiBtoGiB);
                    }
                //}
                //if (azsscanf(buffer, "MemTotal: %lu", &memTotalKiB) && memTotalKiB > 0)
                //{
                    // meminfo displays memory in KiB (kilobytes, base 1024) 
                    // convert to GiB
                    //constexpr double KiBtoGiB = 1024.f * 1024.f;
                    //m_valueInGiB = aznumeric_cast<float>(static_cast<double>(memTotalKiB) / KiBtoGiB);
                    break;
                }
            }
            fclose(f);
        }
    }
} // AzFramework

