/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
*
 */

#include <sys/utsname.h>
#include <AzCore/Console/ConsoleTypeHelpers.h>
#include <AzFramework/Device/DeviceAttributeDeviceModel.h>
#include <AzFramework/Device/DeviceAttributeRAM.h>
#include <Foundation/NSProcessInfo.h>


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
        // the actual RAM amount available to the OS may be less
        // that the physical RAM available, but is not readily available
        constexpr double BytestoGiB = 1024.0 * 1024.0 * 1024.0;
        auto memInBytes = [[NSProcessInfo processInfo] physicalMemory];
        m_valueInGiB = aznumeric_cast<float>(static_cast<double>(memInBytes) / BytestoGiB);
        m_value = AZ::ConsoleTypeHelpers::ValueToString(m_valueInGiB);
    }
} // AzFramework
