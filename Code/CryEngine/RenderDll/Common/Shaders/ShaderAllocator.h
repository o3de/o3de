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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_SHADERS_SHADERALLOCATOR_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_SHADERS_SHADERALLOCATOR_H
#pragma once

#include "CryMemoryAllocator.h"
#include "IMemory.h"

typedef cry_crt_node_allocator ShaderBucketAllocator;

extern ShaderBucketAllocator g_shaderBucketAllocator;
extern IGeneralMemoryHeap* g_shaderGeneralHeap;

template <class T>
class STLShaderAllocator
{
public:
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;
    typedef T*        pointer;
    typedef const T*  const_pointer;
    typedef T&        reference;
    typedef const T&  const_reference;
    typedef T         value_type;

    template <class U>
    struct rebind
    {
        typedef STLShaderAllocator<U> other;
    };

    STLShaderAllocator() throw() { }
    STLShaderAllocator(const STLShaderAllocator&) throw() { }
    template <class U>
    STLShaderAllocator(const STLShaderAllocator<U>&) throw() { }

    pointer address(reference x) const
    {
        return &x;
    }

    const_pointer address(const_reference x) const
    {
        return &x;
    }

    pointer allocate(size_type n = 1, const void* hint = 0)
    {
        pointer ret = NULL;

        (void)hint;
        size_t sz = std::max<size_type>(n * sizeof(T), 1);
        if (sz <= ShaderBucketAllocator::MaxSize)
        {
            ret = static_cast<pointer>(g_shaderBucketAllocator.allocate(sz));
        }
        else
        {
            ret = static_cast<pointer>(g_shaderGeneralHeap->Malloc(sz, NULL));
        }

        return ret;
    }

    void deallocate(pointer p, size_type n = 1)
    {
        (void)n;
        if (p)
        {
            if (g_shaderGeneralHeap && !g_shaderGeneralHeap->Free(p))
            {
                g_shaderBucketAllocator.deallocate(p);
            }
        }
    }

    size_type max_size() const throw()
    {
        return INT_MAX;
    }

    void construct(pointer p, const T& val)
    {
        new(static_cast<void*>(p))T(val);
    }

    void construct(pointer p)
    {
        new(static_cast<void*>(p))T();
    }

    void destroy(pointer p)
    {
        p->~T();
    }

    pointer new_pointer()
    {
        return new(allocate())T();
    }

    pointer new_pointer(const T& val)
    {
        return new(allocate())T(val);
    }

    void delete_pointer(pointer p)
    {
        p->~T();
        deallocate(p);
    }

    bool operator==(const STLShaderAllocator&) const {return true; }
    bool operator!=(const STLShaderAllocator&) const {return false; }

    static void GetMemoryUsage(ICrySizer* pSizer) { }
};

#endif
