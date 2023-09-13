/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
*
 */
#include <sys/types.h> 
#include <sys/sysctl.h> 
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
        int mib[] = {CTL_HW, HW_MEMSIZE};
        int64_t memTotalBytes = 0;
        size_t length = sizeof(memTotalBytes);
        if (sysctl(mib, 2, &memTotalBytes, &length, NULL, 0) == 0)
        {
            constexpr double BytestoGiB = 1024.0 * 1024.0 * 1024.0;
            m_valueInGiB = aznumeric_cast<float>(static_cast<double>(memTotalBytes) / BytestoGiB);
        }
    }
} // AzFramework

