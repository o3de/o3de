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

#ifndef CRYINCLUDE_CRYSYSTEM_PAGEMAPPINGHEAP_H
#define CRYINCLUDE_CRYSYSTEM_PAGEMAPPINGHEAP_H
#pragma once


#include "MemoryAddressRange.h"

#include "IMemory.h"

class CPageMappingHeap
    : public IPageMappingHeap
{
public:
    CPageMappingHeap(char* pAddressSpace, size_t nNumPages, size_t nPageSize, const char* sName);
    CPageMappingHeap(size_t addressSpace, const char* sName);
    ~CPageMappingHeap();

public: // IPageMappingHeap Members
    virtual void Release();

    virtual size_t GetGranularity() const;
    virtual bool IsInAddressRange(void* ptr) const;

    virtual size_t FindLargestFreeBlockSize() const;

    virtual void* Map(size_t sz);
    virtual void Unmap(void* ptr, size_t sz);

private:
    CPageMappingHeap(const CPageMappingHeap&);
    CPageMappingHeap& operator = (const CPageMappingHeap&);

private:
    void Init();

private:
    mutable CryCriticalSectionNonRecursive m_lock;
    CMemoryAddressRange m_addrRange;
    std::vector<uint32> m_pageBitmap;
};

#endif // CRYINCLUDE_CRYSYSTEM_PAGEMAPPINGHEAP_H
