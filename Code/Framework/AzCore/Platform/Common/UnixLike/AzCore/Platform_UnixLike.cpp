/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Platform.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/time.h>

#include <sys/types.h>
#include <unistd.h>

namespace AZ::Platform
{
    ProcessId GetCurrentProcessId()
    {
        return static_cast<ProcessId>(::getpid());
    }

    MachineId GetLocalMachineId()
    {
        if (s_machineId == 0)
        {
            // In specialized server situations, SetLocalMachineId() should be used, with whatever criteria works best in that environment
            // A proper implementation for each supported system will be needed instead of this temporary measure to avoid collision in the small scale.
            // On a larger scale, the odds of two people getting in here at the same millisecond will go up drastically, and we'll have the same issue again,
            // though far less reproducible, for duplicated EntityId's across a network.
            s_machineId = static_cast<AZ::u32>(AZStd::GetTimeUTCMilliSecond() & 0xffffffff);
            if (s_machineId == 0)
            {
                s_machineId = 1;
                AZ_Warning("System", false, "0 machine ID is reserved!");
            }
        }
        return s_machineId;
    }
} // namespace AZ::Platform
