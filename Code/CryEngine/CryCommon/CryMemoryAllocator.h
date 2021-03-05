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
/*
 * Part of this code coming from STLPort alloc
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 *
 */

#ifndef CRYINCLUDE_CRYCOMMON_CRYMEMORYALLOCATOR_H
#define CRYINCLUDE_CRYCOMMON_CRYMEMORYALLOCATOR_H
#pragma once

#include <algorithm>

#define CRY_STL_ALLOC

#if defined(LINUX64) || defined(APPLE)
#include <sys/mman.h>
#endif

#include <string.h>        // memset

// DON't USE _MAX_BYTES as identifier for Max Bytes, STLPORT defines the same enum
// this leads to situation where the wrong enum is choosen in different compilation units
// which in case leads to errors(The stlport one is defined as 128)
#if defined (__OS400__) || defined (_WIN64) || defined(MAC) || defined(LINUX64)
enum {_ALIGNMENT = 16, _ALIGN_SHIFT = 4, __MAX_BYTES = 512, NFREELISTS=32, ADDRESSSPACE = 2 * 1024 * 1024, ADDRESS_SHIFT = 40};
#else
enum {_ALIGNMENT = 8, _ALIGN_SHIFT = 3, __MAX_BYTES = 512, NFREELISTS = 64, ADDRESSSPACE = 2 * 1024 * 1024, ADDRESS_SHIFT = 20};
#endif /* __OS400__ */

#define CRY_MEMORY_ALLOCATOR

#define S_FREELIST_INDEX(__bytes) ((__bytes - size_t(1)) >> (int)_ALIGN_SHIFT)

class  _Node_alloc_obj {
public:
    _Node_alloc_obj * _M_next;
};

#if defined (_WIN64) || defined(APPLE) || defined(LINUX64)
#define MASK_COUNT 0x000000FFFFFFFFFF
#define MASK_VALUE 0xFFFFFF
#define MASK_NEXT 0xFFFFFFFFFF000000
#define MASK_SHIFT 24
#else
#define MASK_COUNT 0x000FFFFF
#define MASK_VALUE 0xFFF
#define MASK_NEXT 0xFFFFF000 
#define MASK_SHIFT 12
#endif

#define NUM_OBJ 64

struct _Obj_Address {
    //    short int * _M_next;
    //    short int 
    size_t GetNext(size_t pBase) {
        return pBase +(size_t)(_M_value >> MASK_SHIFT);
    }

    //size_t GetNext() {
    //    return (size_t)(_M_value >> 20);
    //}

    size_t GetCount() {
        return _M_value & MASK_VALUE;
    }

    void SetNext(/*void **/size_t pNext) {
        _M_value &= MASK_COUNT;
        _M_value |= (size_t)pNext << MASK_SHIFT;
    }

    void SetCount(size_t count) {
        _M_value &= MASK_NEXT;
        _M_value |=  count & MASK_VALUE;
    }
private:
    size_t _M_value;
    //    short int * _M_end;
};

//struct _Node_Allocations_Tree {
//    enum { eListSize = _Size / (sizeof(void *) * _Num_obj); };
//    _Obj_Address * _M_allocations_list[eListSize];
//    int _M_Count;
//    _Node_Allocations_Tree * _M_next;
//};

template<int _Size>
struct _Node_Allocations_Tree {
    //Pointer to the end of the memory block
    char *_M_end;

    enum { eListSize = _Size / (sizeof(void *) * NUM_OBJ) };
    // List of allocations
    _Obj_Address _M_allocations_list[eListSize];
    int _M_allocations_count;
    //Pointer to the next memory block
    _Node_Allocations_Tree *_M_Block_next;
};


struct _Node_alloc_Mem_block_Huge {
    //Pointer to the end of the memory block
    char *_M_end;
    // number 
    int _M_count;
    _Node_alloc_Mem_block_Huge *_M_next;
};

template<int _Size>
struct _Node_alloc_Mem_block {
    //Pointer to the end of the memory block
    char *_M_end;
    //Pointer to the next memory block
    _Node_alloc_Mem_block_Huge *_M_huge_block;
    _Node_alloc_Mem_block *_M_next;
};


// Allocators!
enum EAllocFreeType 
{
    eCryDefaultMalloc,
    eCryMallocCryFreeCRTCleanup,
};

template <EAllocFreeType type>
struct Node_Allocator 
{
    inline void * pool_alloc(size_t size)
    {
        return CryModuleMalloc(size);
    };
    inline void * cleanup_alloc(size_t size) 
    {
        return CryCrtMalloc(size);
    };
    inline size_t pool_free(void * ptr) 
    {
        CryModuleFree(ptr);
        return 0;
    };
    inline void cleanup_free(void * ptr)
    {
        CryCrtFree(ptr);
    };

    inline size_t getSize(void * ptr)
    {
        return CryCrtSize(ptr);
    }
};

// partial 
template <>
struct Node_Allocator<eCryDefaultMalloc>
{
    inline void * pool_alloc(size_t size) 
    {
        return CryCrtMalloc(size);
    };
    inline void * cleanup_alloc(size_t size) 
    {
        return CryCrtMalloc(size);
    };
    inline size_t pool_free(void * ptr) 
    {
        size_t n = CryCrtSize(ptr);
        CryCrtFree(ptr);
        return n;
    };
    inline void cleanup_free(void * ptr)
    {
        CryCrtFree(ptr);
    };
    inline size_t getSize(void * ptr)
    {
        return CryCrtSize(ptr);
    }

};

// partial 
template <>
struct Node_Allocator<eCryMallocCryFreeCRTCleanup>
{
    inline void * pool_alloc(size_t size) 
    {
        return CryCrtMalloc(size);
    };
    inline void * cleanup_alloc(size_t size) 
    {
        return CryCrtMalloc(size);
    };
    inline size_t pool_free(void * ptr) 
    {
        return CryCrtFree(ptr);
    };
    inline void cleanup_free(void * ptr)
    {
        CryCrtFree(ptr);
    };
    inline size_t getSize(void * ptr)
    {
        return CryCrtSize(ptr);
    }

};

#include "MultiThread.h"

struct InternalCriticalSectionDummy {
    char padding[128];
} ;

inline void CryInternalCreateCriticalSection(void * pCS)
{
    CryCreateCriticalSectionInplace(pCS);
}

// A class that forward node allocator calls directly to CRT
struct cry_crt_node_allocator
{
    static const size_t MaxSize = ~0;

    static void *alloc(size_t __n)
    { 
        return CryCrtMalloc(__n);
    }
    static size_t dealloc( void *p )
    { 
        return CryCrtFree(p);
    }
    static void *allocate(size_t __n)
    {
        return alloc(__n);
    }
    static void *allocate(size_t __n, [[maybe_unused]] size_t nAlignment)
    {
        return alloc(__n);
    }
    static size_t deallocate(void *__p)
    {
        return dealloc(__p);
    }
    void cleanup() {}
};


//#endif // WIN32|DEBUG

#endif // CRYINCLUDE_CRYCOMMON_CRYMEMORYALLOCATOR_H
