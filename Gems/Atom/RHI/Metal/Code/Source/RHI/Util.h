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
