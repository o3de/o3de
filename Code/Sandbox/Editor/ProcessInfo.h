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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
