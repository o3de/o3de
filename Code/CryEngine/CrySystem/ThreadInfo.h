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

#ifndef CRYINCLUDE_CRYSYSTEM_THREADINFO_H
#define CRYINCLUDE_CRYSYSTEM_THREADINFO_H
#pragma once


struct SThreadInfo
{
public:
    struct SThreadHandle
    {
        HANDLE Handle;
        uint32 Id;
    };

    typedef std::vector<uint32> TThreadIds;
    typedef std::vector<SThreadHandle> TThreads;
    typedef std::map<uint32, string> TThreadInfo;

    // returns thread info
    static void GetCurrentThreads(TThreadInfo& threadsOut);

    // fills threadsOut vector with thread handles of given thread ids; if threadIds vector is emtpy it fills all running threads
    // if ignoreCurrThread is true it will not return the current thread
    static void OpenThreadHandles(TThreads& threadsOut, const TThreadIds& threadIds = TThreadIds(), bool ignoreCurrThread = true);

    // closes thread handles; should be called whenever GetCurrentThreads was called!
    static void CloseThreadHandles(const TThreads& threads);
};

#endif // CRYINCLUDE_CRYSYSTEM_THREADINFO_H
