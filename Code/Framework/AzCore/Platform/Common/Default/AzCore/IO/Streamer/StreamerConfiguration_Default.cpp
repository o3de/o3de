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
