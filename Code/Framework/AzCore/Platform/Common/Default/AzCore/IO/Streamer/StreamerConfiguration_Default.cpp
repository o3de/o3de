/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>

namespace AZ::IO
{
    bool CollectIoHardwareInformation(
        HardwareInformation& info, [[maybe_unused]] bool includeAllHardware, [[maybe_unused]] bool reportHardware)
    {
        // The numbers below are based on common defaults from a local hardware survey.
        info.m_maxPageSize = 4096;
        info.m_maxTransfer = 512_kib;
        info.m_maxPhysicalSectorSize = 4096;
        info.m_maxLogicalSectorSize = 512;
        info.m_profile = "Generic";
        return true;
    }

    void ReflectNative([[maybe_unused]] ReflectContext* context)
    {
    }
} // namespace AZ::IO
