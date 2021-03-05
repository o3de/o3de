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

#pragma once

#include <AzCore/base.h>

namespace AZ
{
    namespace Platform
    {
        using ProcessId = AZ::u32;
        using MachineId = AZ::u32;

        extern MachineId s_machineId;

        /// Returns the current process id (pid)
        ProcessId GetCurrentProcessId();

        /// Returns a unique machine id that should be unique per platform (PC, Mac, console, etc)
        MachineId GetLocalMachineId();

        /// Sets the machine id. Id should be unique per platform (PC, Mac, console, etc)
        void SetLocalMachineId(MachineId machineId);
    }
}
