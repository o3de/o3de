/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/base.h>

/** Stores information about memory usage of process,
 * All size values are in bytes.
 */

namespace AZ
{
    inline namespace Process
    {
        //! Stores Process Memory information data
        struct ProcessMemInfo
        {
            int64_t m_workingSet{};
            int64_t m_peakWorkingSet{};
            int64_t m_pagefileUsage{};
            int64_t m_peakPagefileUsage{};
            int64_t m_pageFaultCount{};
        };

        /** Retrieve information about memory usage of current process.
         * @param meminfo Output parameter where information is saved.
         */
        bool QueryMemInfo(ProcessMemInfo& meminfo);
    }
}
