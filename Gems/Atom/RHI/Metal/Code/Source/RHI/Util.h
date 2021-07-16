/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
namespace AZ
{
    namespace Metal
    {
        /*
         * Wall time is a colloqial programming term that means elapsed real time.
         * That term differentiates it from CPU time which is the time the process has spent on the CPU.
         * CPU time can add up to more than real time on machines with more than 1 active thread.
         *
         * https://en.m.wikipedia.org/wiki/Elapsed_real_time
         */
        double GetWallTimeSinceApplicationStart();
    }
}
