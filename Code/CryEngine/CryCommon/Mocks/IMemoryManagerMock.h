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
#pragma once
#include <CryMemoryManager.h>

class MemoryManagerMock
    : public IMemoryManager
{
public:
    MOCK_METHOD1(GetProcessMemInfo,
        bool(SProcessMemInfo& minfo));
    MOCK_METHOD3(TraceDefineHeap,
        HeapHandle(const char* heapName, size_t size, const void* pBase));
    MOCK_METHOD6(TraceHeapAlloc,
        void(HeapHandle heap, void* mem, size_t size, size_t blockSize, const char* sUsage, const char* sNameHint));
    MOCK_METHOD3(TraceHeapFree,
        void(HeapHandle heap, void* mem, size_t blockSize));
    MOCK_METHOD1(TraceHeapSetColor,
        void(uint32 color));
    MOCK_METHOD0(TraceHeapGetColor,
        uint32());
    MOCK_METHOD1(TraceHeapSetLabel,
        void(const char* sLabel));
    MOCK_METHOD1(CreateCustomMemoryHeapInstance,
        ICustomMemoryHeap* const (EAllocPolicy const eAllocPolicy));
    MOCK_METHOD3(CreateGeneralExpandingMemoryHeap,
        IGeneralMemoryHeap* (size_t upperLimit, size_t reserveSize, const char* sUsage));
    MOCK_METHOD3(CreateGeneralMemoryHeap,
        IGeneralMemoryHeap* (void* base, size_t sz, const char* sUsage));
    MOCK_METHOD2(ReserveAddressRange,
        IMemoryAddressRange* (size_t capacity, const char* sName));
    MOCK_METHOD2(CreatePageMappingHeap,
        IPageMappingHeap* (size_t addressSpace, const char* sName));
    MOCK_METHOD0(CreateDefragAllocator,
        IDefragAllocator*());
};
