/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

#define ENGINE_NAME "REngine"
#define ENGINE_ORGANIZATION "RStudio"
#define ENGINE_EDITOR_NAME "REngine - Editor"
#define ENGINE_MODE_DESC "Private Mode"
#define ENGINE_EDITOR_SPLASHSCREEN "Starting REngine Editor"
#define ENGINE_ASSET_PROCESSOR_NAME "REngine - Asset Processor"
#define ENGINE_SHADER_MANAGEMENT_NAME "REngine - Shader Management Console"

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
