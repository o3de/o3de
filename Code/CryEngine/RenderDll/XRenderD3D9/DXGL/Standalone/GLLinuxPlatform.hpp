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

// Description : Platform specific DXGL requirements implementation relying
//               on Linux POSIX API


#ifndef __GLLINUXPLATFORM__
#define __GLLINUXPLATFORM__

#include "GLCrossPlatform.hpp"
#include <malloc.h>
#include <pthread.h>
#include <cassert>
#include <sys/stat.h>

#if !defined(TRUE) || !defined(FALSE)
#undef TRUE
#undef FALSE
#define TRUE true
#define FALSE false
#endif //!defined(TRUE) || !defined(FALSE)
#define MAX_PATH 256


namespace NCryOpenGL
{
    namespace NLinuxPlatformImpl
    {
        struct SCriticalSection
        {
            pthread_mutex_t m_kMutex;

            SCriticalSection()
            {
                pthread_mutex_init(&m_kMutex, NULL);
            }

            ~SCriticalSection()
            {
                pthread_mutex_destroy(&m_kMutex);
            }

            void Lock()
            {
                pthread_mutex_lock(&m_kMutex);
            }

            void Unlock()
            {
                pthread_mutex_unlock(&m_kMutex);
            }
        };
    }

    inline void BreakUnique(const char* szFile, uint32 uLine)
    {
        LogMessage(eLS_Warning, "Break at %s(%d)", szFile, uLine);
        __builtin_trap();
    }

    inline LONG Exchange(LONG volatile* piDestination, LONG iExchange)
    {
        LONG iOldValue(__sync_lock_test_and_set(piDestination, iExchange));
        __sync_synchronize();
        return iOldValue;
    }

    inline LONG CompareExchange(LONG volatile* piDestination, LONG iExchange, LONG iComparand)
    {
        return __sync_val_compare_and_swap(piDestination, iComparand, iExchange);
    }

    inline LONG AtomicIncrement(LONG volatile* piDestination)
    {
        return __sync_fetch_and_add(piDestination, 1) + 1;
    }

    inline LONG AtomicDecrement(LONG volatile* piDestination)
    {
        return __sync_fetch_and_sub(piDestination, 1) - 1;
    }

    typedef NLinuxPlatformImpl::SCriticalSection TCriticalSection;

    inline void LockCriticalSection(TCriticalSection* pCriticalSection)
    {
        pCriticalSection->Lock();
    }

    inline void UnlockCriticalSection(TCriticalSection* pCriticalSection)
    {
        pCriticalSection->Unlock();
    }

    struct SLockFreeSingleLinkedListEntry
    {
        SLockFreeSingleLinkedListEntry* volatile pNext;
    } __attribute__((aligned(16)));

    struct SLockFreeSingleLinkedListHeader
    {
        SLockFreeSingleLinkedListEntry* volatile pNext;
        volatile uint64 salt;
    } __attribute__((aligned(16)));

    inline unsigned char _InterlockedCompareExchange128(int64 volatile* dst, int64 exchangehigh, int64 exchangelow, int64* comperand)
    {
        bool bEquals;
        __asm__ __volatile__
        (
            "lock cmpxchg16b %1\n\t"
            "setz %0"
            : "=q" (bEquals), "+m" (*dst), "+d" (comperand[1]), "+a" (comperand[0])
            : "c" (exchangehigh), "b" (exchangelow)
            : "cc"
        );
        return (char)bEquals;
    }

    inline void CryInterlockedPushEntrySList(SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry& element)
    {
        uint64 curSetting[2];
        uint64 newSetting[2];
        uint64 newPointer = (uint64) & element;
        do
        {
            curSetting[0] = (uint64)list.pNext;
            curSetting[1] = list.salt;
            element.pNext = (SLockFreeSingleLinkedListEntry*)curSetting[0];
            newSetting[0] = newPointer; // new pointer
            newSetting[1] = curSetting[1] + 1; // new salt
        }
        while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0]));
    }

    inline void* CryInterlockedPopEntrySList(SLockFreeSingleLinkedListHeader& list)
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
            newSetting[0] = *(uint64*)curSetting[0]; // new pointer
            newSetting[1] = curSetting[1] + 1; // new salt
        }
        while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0]));
        return (void*)curSetting[0];
    }

    inline void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
    {
        list.salt = 0;
        list.pNext = NULL;
    }

    inline void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
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
        while (0 == _InterlockedCompareExchange128((volatile int64*)&list.pNext, (int64)newSetting[1], (int64)newSetting[0], (int64*)&curSetting[0]));
        return (void*)curSetting[0];
    }

    inline void* Memalign(size_t size, size_t alignment)
    {
        return memalign(alignment, size);
    }

    inline void MemalignFree(void* memblock)
    {
        return free(memblock);
    }

    inline bool MakeDir(const char* szDirectory)
    {
        return mkdir(szDirectory, S_IRWXU) == 0;
    }
}

#endif //__GLLINUXPLATFORM__
