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

#ifndef CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_PTHREADS_H
#define CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_PTHREADS_H
#pragma once


#include "CryThread_pthreads.h"

#if PLATFORM_SUPPORTS_THREADLOCAL
THREADLOCAL CrySimpleThreadSelf
* CrySimpleThreadSelf::m_Self = NULL;
#else
TLS_DEFINE(CrySimpleThreadSelf*, g_CrySimpleThreadSelf)
#endif


//////////////////////////////////////////////////////////////////////////
// CryEvent(Timed) implementation
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Reset()
{
    m_lockNotify.Lock();
    m_flag = false;
    m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Set()
{
    m_lockNotify.Lock();
    m_flag = true;
    m_cond.Notify();
    m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
void CryEventTimed::Wait()
{
    m_lockNotify.Lock();
    if (!m_flag)
    {
        m_cond.Wait(m_lockNotify);
    }
    m_flag  =   false;
    m_lockNotify.Unlock();
}

//////////////////////////////////////////////////////////////////////////
bool CryEventTimed::Wait(const uint32 timeoutMillis)
{
    bool bResult = true;
    m_lockNotify.Lock();
    if (!m_flag)
    {
        bResult = m_cond.TimedWait(m_lockNotify, timeoutMillis);
    }
    m_flag  =   false;
    m_lockNotify.Unlock();
    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// CryCriticalSection implementation
///////////////////////////////////////////////////////////////////////////////
typedef CryLockT<CRYLOCK_RECURSIVE> TCritSecType;

void  CryDeleteCriticalSection(void* cs)
{
    delete ((TCritSecType*)cs);
}

void  CryEnterCriticalSection(void* cs)
{
    ((TCritSecType*)cs)->Lock();
}

bool  CryTryCriticalSection(void* cs)
{
    return false;
}

void  CryLeaveCriticalSection(void* cs)
{
    ((TCritSecType*)cs)->Unlock();
}

void  CryCreateCriticalSectionInplace(void* pCS)
{
    new (pCS) TCritSecType;
}

void CryDeleteCriticalSectionInplace(void*)
{
}

void* CryCreateCriticalSection()
{
    return (void*) new TCritSecType;
}

#if AZ_TRAIT_SKIP_CRYINTERLOCKED
#elif defined(INTERLOCKED_COMPARE_EXCHANGE_128_NOT_SUPPORTED)
//////////////////////////////////////////////////////////////////////////
void CryInterlockedPushEntrySList(SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry& element)
{
    AZStd::lock_guard<AZStd::mutex> lock(list.mutex);

    element.pNext = list.pNext;
    list.pNext = &element;
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedPopEntrySList(SLockFreeSingleLinkedListHeader& list)
{
    AZStd::lock_guard<AZStd::mutex> lock(list.mutex);

    SLockFreeSingleLinkedListEntry* returnValue = list.pNext;
    if (list.pNext)
    {
        list.pNext = list.pNext->pNext;
    }
    return returnValue;
}

//////////////////////////////////////////////////////////////////////////
void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
{
    AZStd::lock_guard<AZStd::mutex> lock(list.mutex);

    list.pNext = NULL;
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
{
    AZStd::lock_guard<AZStd::mutex> lock(list.mutex);

    SLockFreeSingleLinkedListEntry* returnValue = list.pNext;
    list.pNext = nullptr;
    return returnValue;
}

#elif defined(LINUX32)
//////////////////////////////////////////////////////////////////////////
// Implementation for Linux32 with gcc using uint64
//////////////////////////////////////////////////////////////////////////
void CryInterlockedPushEntrySList(SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry& element)
{
    uint32 curSetting[2];
    uint32 newSetting[2];
    uint32 newPointer = (uint32) & element;
    do
    {
        curSetting[0] = (uint32)list.pNext;
        curSetting[1] = list.salt;
        element.pNext = (SLockFreeSingleLinkedListEntry*)curSetting[0];
        newSetting[0] = newPointer;     // new pointer
        newSetting[1] = curSetting[1] + 1;   // new salt
    }
    while (false == __sync_bool_compare_and_swap((volatile uint64*)&list.pNext, *(uint64*)&curSetting[0], *(uint64*)&newSetting[0]));
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedPopEntrySList(SLockFreeSingleLinkedListHeader& list)
{
    uint32 curSetting[2];
    uint32 newSetting[2];
    do
    {
        curSetting[1] = list.salt;
        curSetting[0] = (uint32)list.pNext;
        if (curSetting[0] == 0)
        {
            return NULL;
        }
        newSetting[0] = *(uint32*)curSetting[0];     // new pointer
        newSetting[1] = curSetting[1] + 1;   // new salt
    }
    while (false == __sync_bool_compare_and_swap((volatile uint64*)&list.pNext, *(uint64*)&curSetting[0], *(uint64*)&newSetting[0]));
    return (void*)curSetting[0];
}

//////////////////////////////////////////////////////////////////////////
void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
{
    list.salt = 0;
    list.pNext = NULL;
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
{
    uint32 curSetting[2];
    uint32 newSetting[2];
    uint32 newSalt;
    uint32 newPointer;
    do
    {
        curSetting[1] = list.salt;
        curSetting[0] = (uint32)list.pNext;
        if (curSetting[0] == 0)
        {
            return NULL;
        }
        newSetting[0] = 0;
        newSetting[1] = curSetting[1] + 1;
    }
    while (false == __sync_bool_compare_and_swap((volatile uint64*)&list.pNext, *(uint64*)&curSetting[0], *(uint64*)&newSetting[0]));
    return (void*)curSetting[0];
}
#else
// This implementation get's used on multiple platforms that support uint128 compare and swap.

//////////////////////////////////////////////////////////////////////////
// LINUX64 Implementation of Lockless Single Linked List
//////////////////////////////////////////////////////////////////////////
typedef __uint128_t uint128;

//////////////////////////////////////////////////////////////////////////
// Implementation for Linux64 with gcc using __int128_t
//////////////////////////////////////////////////////////////////////////
void CryInterlockedPushEntrySList(SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry& element)
{
    uint64 curSetting[2];
    uint64 newSetting[2];
    uint64 newPointer = (uint64) & element;
    do
    {
        curSetting[0] = (uint64)list.pNext;
        curSetting[1] = list.salt;
        element.pNext = (SLockFreeSingleLinkedListEntry*)curSetting[0];
        newSetting[0] = newPointer;     // new pointer
        newSetting[1] = curSetting[1] + 1;   // new salt
    }
    // while (false == __sync_bool_compare_and_swap( (volatile uint128*)&list.pNext,*(uint128*)&curSetting[0],*(uint128*)&newSetting[0] ));
    while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0]));
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedPopEntrySList(SLockFreeSingleLinkedListHeader& list)
{
    uint64 curSetting[2];
    uint64 newSetting[2];
    do
    {
        curSetting[1] = list.salt;
        curSetting[0] = (uint64)list.pNext;
        if (curSetting[0] == 0)
        {
            return NULL;
        }
        newSetting[0] = *(uint64*)curSetting[0];     // new pointer
        newSetting[1] = curSetting[1] + 1;   // new salt
    }
    //while (false == __sync_bool_compare_and_swap( (volatile uint128*)&list.pNext,*(uint128*)&curSetting[0],*(uint128*)&newSetting[0] ));
    while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0]));
    return (void*)curSetting[0];
}

//////////////////////////////////////////////////////////////////////////
void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
{
    list.salt = 0;
    list.pNext = NULL;
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
{
    uint64 curSetting[2];
    uint64 newSetting[2];
    uint64 newSalt;
    uint64 newPointer;
    do
    {
        curSetting[1] = list.salt;
        curSetting[0] = (uint64)list.pNext;
        if (curSetting[0] == 0)
        {
            return NULL;
        }
        newSetting[0] = 0;
        newSetting[1] = curSetting[1] + 1;
    }
    //  while (false == __sync_bool_compare_and_swap( (volatile uint128*)&list.pNext,*(uint128*)&curSetting[0],*(uint128*)&newSetting[0] ));
    while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0]));
    return (void*)curSetting[0];
}
//////////////////////////////////////////////////////////////////////////
#endif

#endif // CRYINCLUDE_CRYCOMMON_CRYTHREADIMPL_PTHREADS_H
