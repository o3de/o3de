/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::RHI
{
    // Set this variable to true to force the cpu to run in lockstep with the gpu. It will force
    // every RHI::Scope (i.e pass) into its own command list that is explicitly flushed and waited
    // on the cpu after an execute is called. This ensures that an execution for gpu work related to
    // a specific scope (i.e pass) finished successfully on the gpu before the next scope is
    // processed on the cpu. If you can repro a crash in this mode, use it to debug GPU device
    // removals/TDR's when you need to know which scope was executing right before the crash.
    // This variable replaces the old AZ_FORCE_CPU_GPU_INSYNC macro.
    constexpr bool ForceCpuGpuInSync{ false };
} // namespace AZ::RHI
