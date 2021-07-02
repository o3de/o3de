/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "MemoryManager.h"
#include "MultiThreadManager.h"
#include "MemoryTracker.h"


namespace MCore
{
    //
    static Mutex gAlignedMemLock;


    // standard allocate
    void* StandardAllocate(size_t numBytes, uint16 categoryID, uint16 blockID, const char* filename, uint32 lineNr)
    {
        MCORE_UNUSED(categoryID);
        MCORE_UNUSED(blockID);
        MCORE_UNUSED(filename);
        MCORE_UNUSED(lineNr);
        return malloc(numBytes);
    }


    // standard realloc
    void* StandardRealloc(void* memory, size_t numBytes, uint16 categoryID, uint16 blockID, const char* filename, uint32 lineNr)
    {
        MCORE_UNUSED(categoryID);
        MCORE_UNUSED(blockID);
        MCORE_UNUSED(filename);
        MCORE_UNUSED(lineNr);
        return realloc(memory, numBytes);
    }


    // standard free
    void StandardFree(void* memory)
    {
        free(memory);
    }


    // alloc
    void* Allocate(size_t numBytes, uint16 categoryID, uint16 blockID, const char* filename, uint32 lineNr)
    {
        MCore::LockGuard lock(GetMCore().GetMemoryMutex());
        void* result = MCore::GetMCore().GetAllocateFunction()(numBytes, categoryID, blockID, filename, lineNr);
        if (MCore::GetMCore().GetIsTrackingMemory())
        {
            MCore::GetMemoryTracker().RegisterAlloc(result, numBytes, categoryID);
        }

        return result;
    }


    // realloc
    void* Realloc(void* memory, size_t numBytes, uint16 categoryID, uint16 blockID, const char* filename, uint32 lineNr)
    {
        if (memory == nullptr)
        {
            return Allocate(numBytes, categoryID, blockID, filename, lineNr);
        }

        MCore::LockGuard lock(GetMCore().GetMemoryMutex());

        void* result = MCore::GetMCore().GetReallocFunction()(memory, numBytes, categoryID, blockID, filename, lineNr);
        if (MCore::GetMCore().GetIsTrackingMemory())
        {
            MCore::GetMemoryTracker().RegisterRealloc(memory, result, numBytes, categoryID);
        }

        return result;
    }


    // free
    void Free(void* memory)
    {
        if (memory == nullptr)
        {
            return;
        }

        MCore::LockGuard lock(GetMCore().GetMemoryMutex());
        if (MCore::GetMCore().GetIsTrackingMemory())
        {
            MCore::GetMemoryTracker().RegisterFree(memory);
        }

        MCore::GetMCore().GetFreeFunction()(memory);
    }


    // aligned allocate
    void* AlignedAllocate(size_t numBytes, uint16 alignment, uint16 categoryID, uint16 blockID, const char* filename, uint32 lineNr)
    {
        // make sure the alignment value is a power of two
        MCORE_ASSERT(((alignment - 1) & alignment) == 0);

        // check if the alignment is a power of two
        if (((alignment - 1) & alignment) != 0) // if it isn't, we can't continue
        {
            return nullptr;
        }

        uintptr_t ptr = (uintptr_t)Allocate(numBytes + alignment + sizeof(uintptr_t), categoryID, blockID, filename, lineNr); // allocate enough memory for everything
        if (ptr == (uintptr_t)nullptr)
        {
            return nullptr;
        }

        const uintptr_t alignedPtr = (ptr + alignment + sizeof(uintptr_t)) & (~(alignment - 1)); // align the address
        *(uintptr_t*)(alignedPtr - sizeof(uintptr_t)) = ptr; // store the entire memory block address before the aligned block
        return (void*)alignedPtr;
    }


    // aligned realloc
    void* AlignedRealloc(void* alignedAddress, size_t numBytes, size_t prevNumBytes, uint16 alignment, uint16 categoryID, uint16 blockID, const char* filename, uint32 lineNr)
    {
        // if we try to realloc a nullptr pointer
        if (alignedAddress == nullptr)
        {
            return AlignedAllocate(numBytes, alignment, categoryID, blockID, filename, lineNr);
        }

        // make sure the alignment value is a power of two
        MCORE_ASSERT(((alignment - 1) & alignment) == 0);

        // check if the alignment is a power of two
        if (((alignment - 1) & alignment) != 0) // if it isn't, we can't continue
        {
            return nullptr;
        }

        LockGuard lock(gAlignedMemLock);

        // copy the current data into the temp buffer
        // because after the realloc call the memory might be overwritten by another thread or so, before we can copy over the contents
        const size_t numBytesToCopy = (prevNumBytes < numBytes) ? prevNumBytes : numBytes;
        if (numBytesToCopy > 0)
        {
            GetMCore().MemTempBufferAssureSize(numBytesToCopy);
            MemCopy(GetMCore().GetMemTempBuffer(), alignedAddress, numBytesToCopy);
        }

        // extract the unaligned pointer
        void* unalignedPtr = (void*)(*((uintptr_t*)((uintptr_t)alignedAddress - sizeof(uintptr_t))));
        uintptr_t ptr = (uintptr_t)Realloc(unalignedPtr, numBytes + alignment + sizeof(uintptr_t), categoryID, blockID, filename, lineNr);
        if (ptr == (uintptr_t)nullptr)
        {
            return nullptr;
        }

        const uintptr_t alignedPtr = (ptr + alignment + sizeof(uintptr_t)) & (~(alignment - 1)); // align the address
        *(uintptr_t*)(alignedPtr - sizeof(uintptr_t)) = ptr; // store the entire memory block address before the aligned block

        // copy the previous data over, from before the realloc, so that we are sure the contents remain intact
        if (numBytesToCopy > 0)
        {
            MemMove((void*)alignedPtr, GetMCore().GetMemTempBuffer(), numBytesToCopy);
        }

        return (void*)alignedPtr;
    }


    // aligned free
    void AlignedFree(void* alignedAddress)
    {
        if (alignedAddress == nullptr)
        {
            return;
        }

        Free((void*)(*((uintptr_t*)((uintptr_t)alignedAddress - sizeof(uintptr_t))))); // extract the real pointer and free that
    }
}   // namespace MCore


