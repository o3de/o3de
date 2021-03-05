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

#ifndef CRYINCLUDE_CRYCOMMON_ALLOCATOR_H
#define CRYINCLUDE_CRYCOMMON_ALLOCATOR_H
#pragma once

#include "CryMemoryAllocator.h"

////////////////////////////////////////////////////////////////////////
// Allocator default implementation

struct StdAllocator
{
    // Class-specific alloc/free/size functions. Use aligned versions only when necessary.
    template<class T>
    static void* Allocate(T*& p)
    {
        return p = NeedAlign<T>() ?
            (T*)CryModuleMemalign(sizeof(T), alignof(T)) :
            (T*)CryModuleMalloc(sizeof(T));
    }

    template<class T>
    static void Deallocate(T* p)
    {
        if (NeedAlign<T>())
        {
            CryModuleMemalignFree(p);
        }
        else
        {
            CryModuleFree(p);
        }
    }

    template<class T>
    static size_t GetMemSize(const T* p)
    {
        return NeedAlign<T>() ?
               sizeof(T) + alignof(T) :
               sizeof(T);
    }

    template<typename T>
    void GetMemoryUsage(ICrySizer* pSizer) const { /*nothing*/}
protected:

    template<class T>
    static bool NeedAlign()
    { PREFAST_SUPPRESS_WARNING(6326); return alignof(T) > _ALIGNMENT; }
};

// Handy delete template function, for any allocator.
template<class TAlloc, class T>
void Delete(TAlloc& alloc, T* ptr)
{
    if (ptr)
    {
        ptr->~T();
        alloc.Deallocate(ptr);
    }
}

#endif // CRYINCLUDE_CRYCOMMON_ALLOCATOR_H
