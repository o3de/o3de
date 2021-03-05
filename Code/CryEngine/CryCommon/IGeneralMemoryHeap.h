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

#ifndef CRYINCLUDE_CRYCOMMON_IGENERALMEMORYHEAP_H
#define CRYINCLUDE_CRYCOMMON_IGENERALMEMORYHEAP_H
#pragma once

namespace AZ
{
    class IAllocator;
}

class IGeneralMemoryHeap
{
public:
    // <interfuscator:shuffle>
    virtual bool Cleanup() = 0;

    virtual int AddRef() = 0;
    virtual int Release() = 0;

    virtual bool IsInAddressRange(void* ptr) const = 0;

    virtual void* Calloc(size_t nmemb, size_t size, const char* sUsage) = 0;
    virtual void* Malloc(size_t sz, const char* sUsage) = 0;

    // Attempts to free the allocation. Returns the size of the allocation if successful, 0 if the heap doesn't own the address.
    virtual size_t Free(void* ptr) = 0;
    virtual void* Realloc(void* ptr, size_t sz, const char* sUsage) = 0;
    virtual void* ReallocAlign(void* ptr, size_t size, size_t alignment, const char* sUsage) = 0;
    virtual void* Memalign(size_t boundary, size_t size, const char* sUsage) = 0;

    virtual AZ::IAllocator* GetAllocator() const = 0;

    // Get the size of the allocation. Returns 0 if the ptr doesn't belong to the heap.
    virtual size_t UsableSize(void* ptr) const = 0;
    // </interfuscator:shuffle>
protected:
    virtual ~IGeneralMemoryHeap() {}
};

#endif // CRYINCLUDE_CRYCOMMON_IGENERALMEMORYHEAP_H
