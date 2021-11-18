/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Platform.h>

namespace AZ::Platform
{
    MachineId s_machineId = MachineId(0);

    void SetLocalMachineId(AZ::u32 machineId)
    {
        AZ_Assert(machineId != 0, "0 machine ID is reserved!");
        if (s_machineId != 0)
        {
            s_machineId = machineId;
        }
    }
} // namespace AZ::Platform
