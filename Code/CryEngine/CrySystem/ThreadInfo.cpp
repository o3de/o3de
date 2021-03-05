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

#include "CrySystem_precompiled.h"
#include "ThreadInfo.h"
#include "System.h"

////////////////////////////////////////////////////////////////////////////


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define THREADINFO_CPP_SECTION_1 1
#define THREADINFO_CPP_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION THREADINFO_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(ThreadInfo_cpp)
#endif

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#if AZ_LEGACY_CRYSYSTEM_TRAIT_THREADINFO_WINDOWS_STYLE
////////////////////////////////////////////////////////////////////////////
void SThreadInfo::GetCurrentThreads(TThreadInfo& threadsOut)
{
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    DWORD currProcessId = GetCurrentProcessId();
    if (h != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        if (Thread32First(h, &te))
        {
            do
            {
                if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
                {
                    if (te.th32OwnerProcessID == currProcessId)
                    {
                        threadsOut[te.th32ThreadID] = CryThreadGetName(te.th32ThreadID);
                    }
                }
                te.dwSize = sizeof(te);
            } while (Thread32Next(h, &te));
        }
        CloseHandle(h);
    }
}

////////////////////////////////////////////////////////////////////////////
void SThreadInfo::OpenThreadHandles(TThreads& threadsOut, const TThreadIds& threadIds /* = TThreadIds()*/, bool ignoreCurrThread /* = true*/)
{
    TThreadIds threadids = threadIds;
    if (threadids.empty())
    {
        TThreadInfo threads;
        GetCurrentThreads(threads);
        DWORD currThreadId = GetCurrentThreadId();
        for (TThreadInfo::iterator it = threads.begin(), end = threads.end(); it != end; ++it)
        {
            if (!ignoreCurrThread || it->first != currThreadId)
            {
                threadids.push_back(it->first);
            }
        }
    }
    for (TThreadIds::iterator it = threadids.begin(), end = threadids.end(); it != end; ++it)
    {
        SThreadHandle thread;
        thread.Id = *it;
        thread.Handle = OpenThread(THREAD_ALL_ACCESS, FALSE, *it);
        threadsOut.push_back(thread);
    }
}

////////////////////////////////////////////////////////////////////////////
void SThreadInfo::CloseThreadHandles(const TThreads& threads)
{
    for (TThreads::const_iterator it = threads.begin(), end = threads.end(); it != end; ++it)
    {
        CloseHandle(it->Handle);
    }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION THREADINFO_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(ThreadInfo_cpp)
#elif defined(LINUX) || defined(APPLE)
void SThreadInfo::GetCurrentThreads(TThreadInfo& threadsOut)
{
    assert(false); // not implemented!
}

////////////////////////////////////////////////////////////////////////////
void SThreadInfo::OpenThreadHandles(TThreads& threadsOut, const TThreadIds& threadIds /* = TThreadIds()*/, bool ignoreCurrThread /* = true*/)
{
    assert(false); // not implemented!
}

////////////////////////////////////////////////////////////////////////////
void SThreadInfo::CloseThreadHandles(const TThreads& threads)
{
    assert(false); // not implemented!
}

////////////////////////////////////////////////////////////////////////////
#endif
