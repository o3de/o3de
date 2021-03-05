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

#ifndef CRYINCLUDE_CRYCOMMON_IMEMORY_H
#define CRYINCLUDE_CRYCOMMON_IMEMORY_H
#pragma once

#include <smartptr.h>
#include <IDefragAllocator.h> // <> required for Interfuscator
#include <IGeneralMemoryHeap.h> // <> required for Interfuscator
#include <smartptr.h>

struct IMemoryBlock
    : public CMultiThreadRefCount
{
    // <interfuscator:shuffle>
    virtual void* GetData() = 0;
    virtual int GetSize() = 0;
    // </interfuscator:shuffle>
};
TYPEDEF_AUTOPTR(IMemoryBlock);

//////////////////////////////////////////////////////////////////////////
struct ICustomMemoryBlock
    : public IMemoryBlock
{
    // Copy region from from source memory to the specified output buffer
    virtual void CopyMemoryRegion(void* pOutputBuffer, size_t nOffset, size_t nSize) = 0;
};

//////////////////////////////////////////////////////////////////////////
struct ICustomMemoryHeap
    : public CMultiThreadRefCount
{
    // <interfuscator:shuffle>
    virtual ICustomMemoryBlock* AllocateBlock(size_t const nAllocateSize, char const* const sUsage, size_t const nAlignment = 16) = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) = 0;
    virtual size_t GetAllocated() = 0;
    // </interfuscator:shuffle>
};

class IMemoryAddressRange
{
public:
    // <interfuscator:shuffle>
    virtual void Release() = 0;

    virtual char* GetBaseAddress() const = 0;
    virtual size_t GetPageCount() const = 0;
    virtual size_t GetPageSize() const = 0;

    virtual void* MapPage(size_t pageIdx) = 0;
    virtual void UnmapPage(size_t pageIdx) = 0;
    // </interfuscator:shuffle>
protected:
    virtual ~IMemoryAddressRange() {}
};

class IPageMappingHeap
{
public:
    // <interfuscator:shuffle>
    virtual void Release() = 0;

    virtual size_t GetGranularity() const = 0;
    virtual bool IsInAddressRange(void* ptr) const = 0;

    virtual size_t FindLargestFreeBlockSize() const = 0;

    virtual void* Map(size_t sz) = 0;
    virtual void Unmap(void* ptr, size_t sz) = 0;
    // </interfuscator:shuffle>
protected:
    virtual ~IPageMappingHeap() {}
};

#endif // CRYINCLUDE_CRYCOMMON_IMEMORY_H
