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

// Description : Defines functions for CryEngine custom memory manager.

#pragma once

// Section dictionary
#if defined(AZ_RESTRICTED_PLATFORM)
#define CRYMEMORYMANAGER_H_SECTION_TRAITS 1
#define CRYMEMORYMANAGER_H_SECTION_ALLOCPOLICY 2
#endif

#include <AzCore/PlatformRestrictedFileDef.h>
// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYMEMORYMANAGER_H_SECTION_TRAITS
    #include AZ_RESTRICTED_FILE(CryMemoryManager_h)
#else
#if !defined(APPLE)
#define CRYMEMORYMANAGER_H_TRAIT_INCLUDE_MALLOC_H 1
#endif
#if defined(LINUX) || defined(APPLE)
#define CRYMEMORYMANAGER_H_TRAIT_INCLUDE_NEW_NOT_NEW_H 1
#endif
#if !defined(LINUX) && !defined(APPLE)
#define CRYMEMORYMANAGER_H_TRAIT_INCLUDE_CRTDBG_H 1
#endif
#if !defined(APPLE)
#define CRYMEMORYMANAGER_H_TRAIT_USE_CRTCHECKMEMORY 1
#endif
#endif

#include "platform.h"

#include <stdarg.h>
#include <algorithm>

#if defined(APPLE) || defined(ANDROID)
    #include <AzCore/Memory/OSAllocator.h> // memalign
#endif // defined(APPLE)

#ifndef STLALLOCATOR_CLEANUP
#define STLALLOCATOR_CLEANUP
#endif

#define _CRY_DEFAULT_MALLOC_ALIGNMENT 4

#if CRYMEMORYMANAGER_H_TRAIT_INCLUDE_MALLOC_H
    #include <malloc.h>
#endif

#if defined(__cplusplus)
#if CRYMEMORYMANAGER_H_TRAIT_INCLUDE_NEW_NOT_NEW_H
    #include <new>
#else
    #include <new.h>
#endif
#endif

    #ifdef CRYSYSTEM_EXPORTS
        #define CRYMEMORYMANAGER_API DLL_EXPORT
    #else
        #define CRYMEMORYMANAGER_API DLL_IMPORT
    #endif

#ifdef __cplusplus

#if defined(_DEBUG) && CRYMEMORYMANAGER_H_TRAIT_INCLUDE_CRTDBG_H
    #include <crtdbg.h>
#endif //_DEBUG

#include "LegacyAllocator.h"

namespace CryMemory
{
    // checks if the heap is valid in debug; in release, this function shouldn't be called
    // returns non-0 if it's valid and 0 if not valid
    ILINE int IsHeapValid()
    {
    #if (defined(_DEBUG) && !defined(RELEASE_RUNTIME) && CRYMEMORYMANAGER_H_TRAIT_USE_CRTCHECKMEMORY) || (defined(DEBUG_MEMORY_MANAGER))
        return _CrtCheckMemory();
    #else
        return true;
    #endif
    }

    inline void* AllocPages(size_t size)
    {
        const size_t alignment = AZ_PAGE_SIZE;
        void* ret = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(size, alignment, 0, "AllocPages", __FILE__, __LINE__);
        return ret;
    }

    inline void FreePages(void* p, size_t size)
    {
        const size_t alignment = AZ_PAGE_SIZE;
        AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().DeAllocate(p, size, alignment);
    }
}

//////////////////////////////////////////////////////////////////////////

#endif //__cplusplus

struct ICustomMemoryHeap;
class IGeneralMemoryHeap;
class IPageMappingHeap;
class IDefragAllocator;
class IMemoryAddressRange;

// Description:
//   Interfaces that allow access to the CryEngine memory manager.
struct IMemoryManager
{
    typedef unsigned char HeapHandle;
    enum
    {
        BAD_HEAP_HANDLE = 0xFF
    };

    struct SProcessMemInfo
    {
        uint64 PageFaultCount;
        uint64 PeakWorkingSetSize;
        uint64 WorkingSetSize;
        uint64 QuotaPeakPagedPoolUsage;
        uint64 QuotaPagedPoolUsage;
        uint64 QuotaPeakNonPagedPoolUsage;
        uint64 QuotaNonPagedPoolUsage;
        uint64 PagefileUsage;
        uint64 PeakPagefileUsage;

        uint64 TotalPhysicalMemory;
        int64  FreePhysicalMemory;

        uint64 TotalVideoMemory;
        int64 FreeVideoMemory;
    };

    enum EAllocPolicy
    {
        eapDefaultAllocator,
        eapPageMapped,
        eapCustomAlignment,
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYMEMORYMANAGER_H_SECTION_ALLOCPOLICY
    #include AZ_RESTRICTED_FILE(CryMemoryManager_h)
#endif
    };

    virtual ~IMemoryManager(){}

    virtual bool GetProcessMemInfo(SProcessMemInfo& minfo) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Heap Tracing API
    virtual HeapHandle TraceDefineHeap(const char* heapName, size_t size, const void* pBase) = 0;
    virtual void TraceHeapAlloc(HeapHandle heap, void* mem, size_t size, size_t blockSize, const char* sUsage, const char* sNameHint = 0) = 0;
    virtual void TraceHeapFree(HeapHandle heap, void* mem, size_t blockSize) = 0;
    virtual void TraceHeapSetColor(uint32 color) = 0;
    virtual uint32 TraceHeapGetColor() = 0;
    virtual void TraceHeapSetLabel(const char* sLabel) = 0;
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of ICustomMemoryHeap
    virtual ICustomMemoryHeap* const CreateCustomMemoryHeapInstance(EAllocPolicy const eAllocPolicy) = 0;
    virtual IGeneralMemoryHeap* CreateGeneralExpandingMemoryHeap(size_t upperLimit, size_t reserveSize, const char* sUsage) = 0;
    virtual IGeneralMemoryHeap* CreateGeneralMemoryHeap(void* base, size_t sz, const char* sUsage) = 0;

    virtual IMemoryAddressRange* ReserveAddressRange(size_t capacity, const char* sName) = 0;
    virtual IPageMappingHeap* CreatePageMappingHeap(size_t addressSpace, const char* sName) = 0;

    virtual IDefragAllocator* CreateDefragAllocator() = 0;
};

// Global function implemented in CryMemoryManager_impl.h
IMemoryManager* CryGetIMemoryManager();

// Summary:
//      Structure filled by call to CryModuleGetMemoryInfo().
struct CryModuleMemoryInfo
{
    uint64 requested;
    // Total Ammount of memory allocated.
    uint64 allocated;
    // Total Ammount of memory freed.
    uint64 freed;
    // Total number of memory allocations.
    int num_allocations;
    // Allocated in CryString.
    uint64 CryString_allocated;
    // Allocated in STL.
    uint64 STL_allocated;
    // Amount of memory wasted in pools in stl (not usefull allocations).
    uint64 STL_wasted;
};

struct CryReplayInfo
{
    uint64 uncompressedLength;
    uint64 writtenLength;
    uint32 trackingSize;
    const char* filename;
};

//////////////////////////////////////////////////////////////////////////
// Extern declarations of globals inside CrySystem.
//////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


void* CryMalloc(size_t size, size_t& allocated, size_t alignment);
void* CryRealloc(void* memblock, size_t size, size_t& allocated, size_t& oldsize, size_t alignment);
size_t CryFree(void* p, size_t alignment);
size_t CryGetMemSize(void* p, size_t size);
int  CryStats(char* buf);
void CryFlushAll();
void CryCleanup();
int  CryGetUsedHeapSize();
int  CryGetWastedHeapSize();
size_t CrySystemCrtGetUsedSpace();
CRYMEMORYMANAGER_API void CryGetIMemoryManagerInterface(void** pIMemoryManager);

#ifdef __cplusplus
}
#endif //__cplusplus

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Cry Memory Manager accessible in all build modes.
//////////////////////////////////////////////////////////////////////////
#if !defined(USING_CRY_MEMORY_MANAGER)
#define USING_CRY_MEMORY_MANAGER
#endif

#include "CryLegacyAllocator.h"


template<typename T, typename ... Args>
inline T* CryAlignedNew(Args&& ... args)
{
    void* pAlignedMemory = CryModuleMemalign(sizeof(T), std::alignment_of<T>::value);
    return new(pAlignedMemory) T(std::forward<Args>(args) ...);
}

// This utility function should be used for allocating arrays of objects with specific alignment requirements on the heap.
// Note: The caller must remember the number of items in the array, since CryAlignedDeleteArray needs this information.
template<typename T>
inline T* CryAlignedNewArray(size_t count)
{
    T* const pAlignedMemory = reinterpret_cast<T*>(CryModuleMemalign(sizeof(T) * count, std::alignment_of<T>::value));
    T* pCurrentItem = pAlignedMemory;
    for (size_t i = 0; i < count; ++i, ++pCurrentItem)
    {
        new(static_cast<void*>(pCurrentItem))T();
    }
    return pAlignedMemory;
}

// Utility function that frees an object previously allocated with CryAlignedNew.
template<typename T>
inline void CryAlignedDelete(T* pObject)
{
    if (pObject)
    {
        pObject->~T();
        CryModuleMemalignFree(pObject);
    }
}

// Utility function that frees an array of objects previously allocated with CryAlignedNewArray.
// The same count used to allocate the array must be passed to this function.
template<typename T>
inline void CryAlignedDeleteArray(T* pObject, size_t count)
{
    if (pObject)
    {
        for (size_t i = 0; i < count; ++i)
        {
            (pObject + i)->~T();
        }
        CryModuleMemalignFree(pObject);
    }
}
