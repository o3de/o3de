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

#ifndef CRYINCLUDE_CRYCOMMON_ISYSTEMSCHEDULER_H
#define CRYINCLUDE_CRYCOMMON_ISYSTEMSCHEDULER_H
#pragma once

#if defined(__cplusplus)
#define SLICE_AND_SLEEP() do { if (GetISystemScheduler()) { GetISystemScheduler()->SliceAndSleep(__FUNC__, __LINE__); } \
} while (0)
#define SLICE_SCOPE_DEFINE() CSliceLoadingMonitor sliceScope
#else
extern void SliceAndSleep(const char* pFunc, int line);
#define SLICE_AND_SLEEP() SliceAndSleep(__FILE__, __LINE__)
#endif

struct ISystemScheduler
{
    virtual ~ISystemScheduler(){}

    // <interfuscator:shuffle>
    // Map load slicing functionality support
    virtual void SliceAndSleep(const char* sliceName, int line) = 0;
    virtual void SliceLoadingBegin() = 0;
    virtual void SliceLoadingEnd() = 0;

    virtual void SchedulingSleepIfNeeded(void) = 0;
    // </interfuscator:shuffle>
};

ISystemScheduler* GetISystemScheduler(void);

class CSliceLoadingMonitor
{
public:
    CSliceLoadingMonitor()
    {
        if (GetISystemScheduler())
        {
            GetISystemScheduler()->SliceLoadingBegin();
        }
    }

    ~CSliceLoadingMonitor()
    {
        if (GetISystemScheduler())
        {
            GetISystemScheduler()->SliceLoadingEnd();
        }
    }
};

#endif // CRYINCLUDE_CRYCOMMON_ISYSTEMSCHEDULER_H
