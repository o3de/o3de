/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/BestFitExternalMapSchema.h>
#include <AzCore/Memory/SystemAllocator.h>

using namespace AZ;

//=========================================================================
// BestFitExternalMapSchema
// [1/28/2011]
//=========================================================================
BestFitExternalMapSchema::BestFitExternalMapSchema(const Descriptor& desc)
    : m_desc(desc)
    , m_used(0)
    , m_freeChunksMap(FreeMapType::key_compare(), AZStdIAllocator(desc.m_mapAllocator != nullptr ? desc.m_mapAllocator : &AllocatorInstance<SystemAllocator>::Get()))
    , m_allocChunksMap(AllocMapType::hasher(), AllocMapType::key_eq(), AZStdIAllocator(desc.m_mapAllocator != nullptr ? desc.m_mapAllocator : &AllocatorInstance<SystemAllocator>::Get()))
{
    if (m_desc.m_mapAllocator == nullptr)
    {
        m_desc.m_mapAllocator = &AllocatorInstance<SystemAllocator>::Get(); // used as our sub allocator
    }
    AZ_Assert(m_desc.m_memoryBlockByteSize > 0, "You must provide memory block size!");
    AZ_Assert(m_desc.m_memoryBlock != nullptr, "You must provide memory block allocated as you with!");
    //if( m_desc.m_memoryBlock == NULL) there is no point to automate this cause we need to flag this memory special, otherwise there is no point to use this allocator at all
    //  m_desc.m_memoryBlock = azmalloc(SystemAllocator,m_desc.m_memoryBlockByteSize,16);
    m_freeChunksMap.insert(AZStd::make_pair(m_desc.m_memoryBlockByteSize, reinterpret_cast<char*>(m_desc.m_memoryBlock)));
}

//=========================================================================
// Allocate
// [1/28/2011]
//=========================================================================
BestFitExternalMapSchema::pointer_type
BestFitExternalMapSchema::Allocate(size_type byteSize, size_type alignment, int flags)
{
    (void)flags;
    char* address = nullptr;
    AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "Alignment must be >0 and power of 2!");
    for (int i = 0; i < 2; ++i) // max 2 attempts to allocate
    {
        FreeMapType::iterator iter = m_freeChunksMap.find(byteSize);
        size_t  blockSize = 0;
        char*   blockAddress = nullptr;
        size_t  preAllocBlockSize = 0;
        while (iter != m_freeChunksMap.end())
        {
            blockSize = iter->first;
            blockAddress = iter->second;
            char* alignedAddr = PointerAlignUp(blockAddress, alignment);
            preAllocBlockSize = alignedAddr - blockAddress;
            if (preAllocBlockSize + byteSize <= blockSize)
            {
                m_freeChunksMap.erase(iter); // we have our allocation
                m_used += byteSize;
                address = alignedAddr;
                m_allocChunksMap.insert(AZStd::make_pair(address, byteSize));
                break;
            }
            ++iter;
        }
        if (address != nullptr)
        {
            // split blocks
            if (preAllocBlockSize)  // if we have a block before the alignment
            {
                m_freeChunksMap.insert(AZStd::make_pair(preAllocBlockSize, blockAddress));
            }
            size_t postAllocBlockSize = blockSize - preAllocBlockSize - byteSize;
            if (postAllocBlockSize)
            {
                m_freeChunksMap.insert(AZStd::make_pair(postAllocBlockSize, address + byteSize));
            }

            break;
        }
        else
        {
            GarbageCollect();
        }
    }
    return address;
}

//=========================================================================
// DeAllocate
// [1/28/2011]
//=========================================================================
void
BestFitExternalMapSchema::DeAllocate(pointer_type ptr)
{
    if (ptr == nullptr)
    {
        return;
    }
    AllocMapType::iterator iter = m_allocChunksMap.find(reinterpret_cast<char*>(ptr));
    if (iter != m_allocChunksMap.end())
    {
        m_used -= iter->second;
        m_freeChunksMap.insert(AZStd::make_pair(iter->second, iter->first));
        m_allocChunksMap.erase(iter);
    }
}

//=========================================================================
// AllocationSize
// [1/28/2011]
//=========================================================================
BestFitExternalMapSchema::size_type
BestFitExternalMapSchema::AllocationSize(pointer_type ptr)
{
    AllocMapType::iterator iter = m_allocChunksMap.find(reinterpret_cast<char*>(ptr));
    if (iter != m_allocChunksMap.end())
    {
        return iter->second;
    }
    return 0;
}

//=========================================================================
// GetMaxAllocationSize
// [1/28/2011]
//=========================================================================
BestFitExternalMapSchema::size_type
BestFitExternalMapSchema::GetMaxAllocationSize() const
{
    if (!m_freeChunksMap.empty())
    {
        return m_freeChunksMap.rbegin()->first;
    }
    return 0;
}

auto BestFitExternalMapSchema::GetMaxContiguousAllocationSize() const -> size_type
{
    // Return the maximum size of any single allocation
    return AZ_CORE_MAX_ALLOCATOR_SIZE;
}

//=========================================================================
// GarbageCollect
// [1/28/2011]
//=========================================================================
void
BestFitExternalMapSchema::GarbageCollect()
{
    for (FreeMapType::iterator curBlock = m_freeChunksMap.begin(); curBlock != m_freeChunksMap.end(); )
    {
        char* curStart = curBlock->second;
        char* curEnd = curStart + curBlock->first;
        bool isMerge = false;
        for (FreeMapType::iterator nextBlock = curBlock++; nextBlock != m_freeChunksMap.end(); )
        {
            char* nextStart = nextBlock->second;
            char* nextEnd = nextStart + nextBlock->first;
            if (curStart == nextEnd)
            {
                // merge
                size_t newBlockSize = curBlock->first + nextBlock->first;
                char* newBlockAddress = nextStart;
                m_freeChunksMap.erase(nextBlock);
                FreeMapType::iterator toErase = curBlock;
                ++curBlock;
                m_freeChunksMap.erase(toErase);
                FreeMapType::iterator newBlock = m_freeChunksMap.insert(AZStd::make_pair(newBlockSize, newBlockAddress)).first;
                if (curBlock != m_freeChunksMap.end() && newBlockSize < curBlock->first)  // if the newBlock in before the next in the list, update next in the list to current
                {
                    curBlock = newBlock;
                }
                isMerge = true;
                break;
            }
            else if (curEnd == nextStart)
            {
                // merge
                size_t newBlockSize = curBlock->first + nextBlock->first;
                char* newBlockAddress = curStart;
                m_freeChunksMap.erase(nextBlock);
                FreeMapType::iterator toErase = curBlock;
                ++curBlock;
                m_freeChunksMap.erase(toErase);
                FreeMapType::iterator newBlock = m_freeChunksMap.insert(AZStd::make_pair(newBlockSize, newBlockAddress)).first;
                if (curBlock != m_freeChunksMap.end() && newBlockSize < curBlock->first)  // if the newBlock in before the next in the list, update next in the list to current
                {
                    curBlock = newBlock;
                }
                isMerge = true;
                break;
            }
            ++nextBlock;
        }
        if (!isMerge)
        {
            ++curBlock;
        }
    }
}
