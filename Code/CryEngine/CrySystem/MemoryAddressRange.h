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

#ifndef CRYINCLUDE_CRYSYSTEM_MEMORYADDRESSRANGE_H
#define CRYINCLUDE_CRYSYSTEM_MEMORYADDRESSRANGE_H
#pragma once


#include "IMemory.h"

class CMemoryAddressRange
    : public IMemoryAddressRange
{
public:
    static void* ReserveSpace(size_t sz);
    static size_t GetSystemPageSize();

public:
    CMemoryAddressRange(char* pBaseAddress, size_t nPageSize, size_t nPageCount, const char* sName);
    CMemoryAddressRange(size_t capacity, const char* name);
    ~CMemoryAddressRange();

    ILINE bool IsInRange(void* p) const
    {
        return m_pBaseAddress <= p && p < (m_pBaseAddress + m_nPageSize * m_nPageCount);
    }

public:
    void Release();

    char* GetBaseAddress() const;
    size_t GetPageCount() const;
    size_t GetPageSize() const;

    void* MapPage(size_t pageIdx);
    void UnmapPage(size_t pageIdx);

private:
    CMemoryAddressRange(const CMemoryAddressRange&);
    CMemoryAddressRange& operator = (const CMemoryAddressRange&);

private:
    char* m_pBaseAddress;
    size_t m_nPageSize;
    size_t m_nPageCount;
#if defined(APPLE) || defined(LINUX)
    size_t m_allocatedSpace; // Required to unmap latter on
#endif
};

#endif // CRYINCLUDE_CRYSYSTEM_MEMORYADDRESSRANGE_H
