/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if defined(APPLE) || defined(LINUX)
#include <sched.h>
#endif

#include <AzCore/std/parallel/mutex.h>

#include "CryAssert.h"

// Section dictionary
#if defined(AZ_RESTRICTED_PLATFORM)
#define MULTITHREAD_H_SECTION_TRAITS 1
#define MULTITHREAD_H_SECTION_DEFINE_CRYINTERLOCKEXCHANGE 2
#define MULTITHREAD_H_SECTION_IMPLEMENT_CRYSPINLOCK 3
#define MULTITHREAD_H_SECTION_IMPLEMENT_CRYINTERLOCKEDADD 4
#define MULTITHREAD_H_SECTION_IMPLEMENT_CRYINTERLOCKEDADDSIZE 5
#define MULTITHREAD_H_SECTION_CRYINTERLOCKEDFLUSHSLIST_PT1 6
#define MULTITHREAD_H_SECTION_CRYINTERLOCKEDFLUSHSLIST_PT2 7
#define MULTITHREAD_H_SECTION_IMPLEMENT_CRYINTERLOCKEDCOMPAREEXCHANGE64 8
#endif

#define WRITE_LOCK_VAL (1 << 16)

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION MULTITHREAD_H_SECTION_TRAITS
    #include AZ_RESTRICTED_FILE(MultiThread_h)
#endif

void CrySpinLock(volatile int* pLock, int checkVal, int setVal);
void CryReleaseSpinLock   (volatile int*, int);

LONG   CryInterlockedIncrement(int volatile* lpAddend);
LONG   CryInterlockedDecrement(int volatile* lpAddend);
LONG   CryInterlockedOr(LONG volatile* Destination, LONG Value);
LONG   CryInterlockedExchangeAdd(LONG volatile* lpAddend, LONG Value);
LONG     CryInterlockedCompareExchange(LONG volatile* dst, LONG exchange, LONG comperand);
void*    CryInterlockedCompareExchangePointer(void* volatile* dst, void* exchange, void* comperand);
void*  CryInterlockedExchangePointer       (void* volatile* dst, void* exchange);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION MULTITHREAD_H_SECTION_DEFINE_CRYINTERLOCKEXCHANGE
    #include AZ_RESTRICTED_FILE(MultiThread_h)
#endif

ILINE void CrySpinLock(volatile int* pLock, int checkVal, int setVal)
{
#ifdef _CPU_X86
# ifdef __GNUC__
    int val;
    __asm__ __volatile__ (
        "0:     mov %[checkVal], %%eax\n"
        "       lock cmpxchg %[setVal], (%[pLock])\n"
        "       jnz 0b"
        : "=m" (*pLock)
        : [pLock] "r" (pLock), "m" (*pLock),
        [checkVal] "m" (checkVal),
        [setVal] "r" (setVal)
        : "eax", "cc", "memory"
        );
# else //!__GNUC__
    __asm
    {
        mov edx, setVal
        mov ecx, pLock
Spin:
        // Trick from Intel Optimizations guide
#ifdef _CPU_SSE
            pause
#endif
        mov eax, checkVal
        lock cmpxchg [ecx], edx
        jnz Spin
    }
# endif //!__GNUC__
#else // !_CPU_X86
# if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION MULTITHREAD_H_SECTION_IMPLEMENT_CRYSPINLOCK
    #include AZ_RESTRICTED_FILE(MultiThread_h)
# endif
# if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#  undef AZ_RESTRICTED_SECTION_IMPLEMENTED
# elif defined(APPLE) || defined(LINUX)
    //    register int val;
    //  __asm__ __volatile__ (
    //      "0:     mov %[checkVal], %%eax\n"
    //      "       lock cmpxchg %[setVal], (%[pLock])\n"
    //      "       jnz 0b"
    //      : "=m" (*pLock)
    //      : [pLock] "r" (pLock), "m" (*pLock),
    //        [checkVal] "m" (checkVal),
    //        [setVal] "r" (setVal)
    //      : "eax", "cc", "memory"
    //      );
    //while(CryInterlockedCompareExchange((volatile long*)pLock,setVal,checkVal)!=checkVal) ;
    uint loops = 0;
    while (__sync_val_compare_and_swap((volatile int32_t*)pLock, (int32_t)checkVal, (int32_t)setVal) != checkVal)
    {
#   if !defined (ANDROID) && !defined(IOS)
        _mm_pause();
#   endif

        if (!(++loops & 0x7F))
        {
            usleep(1); // give threads with other prio chance to run
        }
        else if (!(loops & 0x3F))
        {
            sched_yield(); // give threads with same prio chance to run
        }
    }
# else
    // NOTE: The code below will fail on 64bit architectures!
    while (_InterlockedCompareExchange((volatile LONG*)pLock, setVal, checkVal) != checkVal)
    {
        _mm_pause();
    }
# endif
#endif
}

ILINE void CryReleaseSpinLock(volatile int* pLock, int setVal)
{
    *pLock = setVal;
}

//////////////////////////////////////////////////////////////////////////
ILINE void CryInterlockedAdd(volatile int* pVal, int iAdd)
{
#ifdef _CPU_X86
# ifdef __GNUC__
    __asm__ __volatile__ (
        "        lock add %[iAdd], (%[pVal])\n"
        : "=m" (*pVal)
        : [pVal] "r" (pVal), "m" (*pVal), [iAdd] "r" (iAdd)
        );
# else
    __asm
    {
        mov edx, pVal
        mov eax, iAdd
        lock add [edx], eax
    }
# endif
#else
    // NOTE: The code below will fail on 64bit architectures!
#if defined(_WIN64)
    _InterlockedExchangeAdd((volatile LONG*)pVal, iAdd);
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION MULTITHREAD_H_SECTION_IMPLEMENT_CRYINTERLOCKEDADD
    #include AZ_RESTRICTED_FILE(MultiThread_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(APPLE) || defined(LINUX)
    CryInterlockedExchangeAdd((volatile LONG*)pVal, iAdd);
#elif defined(APPLE)
    OSAtomicAdd32(iAdd, (volatile LONG*)pVal);
#else
    InterlockedExchangeAdd((volatile LONG*)pVal, iAdd);
#endif

#endif
}

ILINE void CryInterlockedAddSize(volatile size_t* pVal, ptrdiff_t iAdd)
{
#if defined(PLATFORM_64BIT)
#if defined(_WIN64)
    _InterlockedExchangeAdd64((volatile __int64*)pVal, iAdd);
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION MULTITHREAD_H_SECTION_IMPLEMENT_CRYINTERLOCKEDADDSIZE
    #include AZ_RESTRICTED_FILE(MultiThread_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32)
    InterlockedExchangeAdd64((volatile LONG64*)pVal, iAdd);
#elif defined(APPLE) || defined(LINUX)
    (void)__sync_fetch_and_add((int64_t*)pVal, (int64_t)iAdd);
#else
    int64 x, n;
    do
    {
        x = (int64) * pVal;
        n = x + iAdd;
    }
    while (CryInterlockedCompareExchange64((volatile int64*)pVal, n, x) != x);
#endif
#else
    CryInterlockedAdd((volatile int*)pVal, (int)iAdd);
#endif
}



//////////////////////////////////////////////////////////////////////////
ILINE void CryWriteLock(volatile int* rw)
{
    CrySpinLock(rw, 0, WRITE_LOCK_VAL);
}

ILINE void CryReleaseWriteLock(volatile int* rw)
{
    CryInterlockedAdd(rw, -WRITE_LOCK_VAL);
}

//////////////////////////////////////////////////////////////////////////
struct WriteLock
{
    ILINE WriteLock(volatile int& rw) { CryWriteLock(&rw); prw = &rw; }
    ~WriteLock() { CryReleaseWriteLock(prw); }
private:
    volatile int* prw;
};

//////////////////////////////////////////////////////////////////////////
struct WriteLockCond
{
    ILINE WriteLockCond(volatile int& rw, int bActive = 1)
    {
        if (bActive)
        {
            CrySpinLock(&rw, 0, iActive = WRITE_LOCK_VAL);
        }
        else
        {
            iActive = 0;
        }
        prw = &rw;
    }
    ILINE WriteLockCond() { prw = &(iActive = 0); }
    ~WriteLockCond()
    {
        CryInterlockedAdd(prw, -iActive);
    }
    void SetActive(int bActive = 1) { iActive = -bActive & WRITE_LOCK_VAL; }
    void Release() { CryInterlockedAdd(prw, -iActive); }
    volatile int* prw;
    int iActive;
};


#if defined(LINUX) || defined(APPLE)
ILINE int64 CryInterlockedCompareExchange64(volatile int64* addr, int64 exchange, int64 comperand)
{
    return __sync_val_compare_and_swap(addr, comperand, exchange);
    // This is OK, because long is signed int64 on Linux x86_64
    //return CryInterlockedCompareExchange((volatile long*)addr, (long)exchange, (long)comperand);
}
#else
ILINE int64 CryInterlockedCompareExchange64(volatile int64* addr, int64 exchange, int64 compare)
{
    // forward to system call
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION MULTITHREAD_H_SECTION_IMPLEMENT_CRYINTERLOCKEDCOMPAREEXCHANGE64
    #include AZ_RESTRICTED_FILE(MultiThread_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    return _InterlockedCompareExchange64((volatile int64*)addr, exchange, compare);
#endif
}
#endif
