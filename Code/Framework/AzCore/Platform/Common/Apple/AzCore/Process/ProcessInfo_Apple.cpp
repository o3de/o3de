/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Process/ProcessInfo.h>

#include <mach/mach.h>

namespace AZ
{
    inline namespace Process
    {
        bool QueryMemInfo(ProcessMemInfo& meminfo)
        {
            memset(&meminfo, 0, sizeof(ProcessMemInfo));

            task_basic_info basic_info;
            mach_msg_type_number_t size = sizeof(basic_info);
            kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&basic_info), &size);
            if (kerr != KERN_SUCCESS)
            {
                return false;
            }

            meminfo.m_workingSet = basic_info.resident_size;
            meminfo.m_pagefileUsage = basic_info.virtual_size;

            task_events_info events_info;
            size = sizeof(events_info);
            kerr = task_info(mach_task_self(), TASK_EVENTS_INFO, reinterpret_cast<task_info_t>(&events_info), &size);
            if (kerr == KERN_SUCCESS)
            {
                meminfo.m_pageFaultCount = events_info.faults;
            }

            return kerr == KERN_SUCCESS;
        }
    } // inline namespace Process
} // namespace AZ
