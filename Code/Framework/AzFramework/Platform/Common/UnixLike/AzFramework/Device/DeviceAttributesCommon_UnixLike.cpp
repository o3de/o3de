/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
*
 */

#include <sys/utsname.h>
#include <AzFramework/Device/DeviceAttributes.h>

namespace AzFramework
{
    DeviceAttributeDeviceModel::DeviceAttributeDeviceModel()
    {
        m_value = 0;
        utsname systemInfo;
        if (uname(&systemInfo) != -1)
        {
            m_value = systemInfo.machine;
        }
    }

    DeviceAttributeRAM::DeviceAttributeRAM()
    {
        m_value = 0;

        FILE* f;
        azfopen(&f, "/proc/meminfo", "r");
        if (f)
        {
            char buffer[256] = { 0 };
            int total = 0;
            while (fgets(buffer, sizeof(buffer), f))
            {
                if (azsscanf(buffer, "MemTotal: %d", &total))
                {
                    // meminfo displays memory in kB
                    constexpr float div = 1024.f * 1024.f;
                    m_value = aznumeric_cast<float>(total) / div;
                }
            }
            fclose(f);
        }
    }
} // AzFramework

