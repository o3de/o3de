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

#ifndef CRYINCLUDE_CRYSYSTEM_MEMORYMANAGER_H
#define CRYINCLUDE_CRYSYSTEM_MEMORYMANAGER_H
#pragma once

#include "ISystem.h"

//////////////////////////////////////////////////////////////////////////
// Class that implements IMemoryManager interface.
//////////////////////////////////////////////////////////////////////////
#ifndef MEMMAN_STATIC
class CCryMemoryManager
    : public IMemoryManager
{
public:
    // Singleton
    static CCryMemoryManager* GetInstance();

    //////////////////////////////////////////////////////////////////////////
    virtual bool GetProcessMemInfo(SProcessMemInfo& minfo);

    virtual HeapHandle TraceDefineHeap(const char* heapName, size_t size, const void* pBase);
    virtual void TraceHeapAlloc(HeapHandle heap, void* mem, size_t size, size_t blockSize, const char* sUsage, const char* sNameHint = 0);
    virtual void TraceHeapFree(HeapHandle heap, void* mem, size_t blockSize);
    virtual void TraceHeapSetColor(uint32 color);
    virtual uint32 TraceHeapGetColor();
    virtual void TraceHeapSetLabel(const char* sLabel);

    virtual ICustomMemoryHeap* const CreateCustomMemoryHeapInstance(IMemoryManager::EAllocPolicy const eAllocPolicy);
    virtual IGeneralMemoryHeap* CreateGeneralExpandingMemoryHeap(size_t upperLimit, size_t reserveSize, const char* sUsage);
    virtual IGeneralMemoryHeap* CreateGeneralMemoryHeap(void* base, size_t sz, const char* sUsage);

    virtual IMemoryAddressRange* ReserveAddressRange(size_t capacity, const char* sName);
    virtual IPageMappingHeap* CreatePageMappingHeap(size_t addressSpace, const char* sName);

    virtual IDefragAllocator* CreateDefragAllocator();
};
#else
typedef IMemoryManager CCryMemoryManager;
#endif

#endif // CRYINCLUDE_CRYSYSTEM_MEMORYMANAGER_H
