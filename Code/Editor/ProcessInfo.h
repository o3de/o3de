/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_PROCESSINFO_H
#define CRYINCLUDE_EDITOR_PROCESSINFO_H

#pragma once

/** Stores information about memory usage of process, retrieved from CProcessInfo class.
        All size values are in bytes.
*/
struct ProcessMemInfo
{
    int64 WorkingSet;
    int64 PeakWorkingSet;
    int64 PagefileUsage;
    int64 PeakPagefileUsage;
    int64 PageFaultCount;
};

/** Use this class to query information about current process.
        Like memory usage, pagefile usage etc..
*/
class SANDBOX_API CProcessInfo
{
public:
    CProcessInfo(void);
    ~CProcessInfo(void);

    /** Retrieve information about memory usage of current process.
            @param meminfo Output parameter where information is saved.
    */
    static void QueryMemInfo(ProcessMemInfo& meminfo);
};

#endif // CRYINCLUDE_EDITOR_PROCESSINFO_H
