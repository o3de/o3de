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
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/Math/MathUtils.h>
// include core related headers
#include "MemoryCategoriesCore.h"
#include "StandardHeaders.h"
#include "MCoreSystem.h"

namespace MCore
{
    /**
     * Enable or disable the use of the memory manager.
     */
#define MCORE_USE_MEMORYMANAGER


    // if we want to use the memory manager
#ifdef MCORE_USE_MEMORYMANAGER
    /**
     * Specify that a given class has to be managed by the memory manager.
     * @param CLASSNAME The class name, which would be "FooBar" in case you declare your class as "class FooBar { .... };".
     * @param ALIGNMENT The alignment, in bytes, on which the memory address should be aligned when you 'new' a class of this type.
     * @param CATEGORY An integer containing the memory category ID where this class belongs to.
     */
    #define MCORE_MEMORYOBJECTCATEGORY(CLASSNAME, ALIGNMENT, CATEGORY)                           \
public:                                                                                          \
    static uint16 GetMemoryCategory() { return CATEGORY; }                                       \
    static uint16 GetMemoryAlignment()                                                           \
    {                                                                                            \
        return AZ::GetMax<uint16>(ALIGNMENT, AZStd::alignment_of<CLASSNAME>::value);             \
    }                                                                                            \
                                                                                                 \
    void* operator new(size_t numBytes)                                                          \
    {                                                                                            \
        const uint16 category  = GetMemoryCategory();                                            \
        const uint16 alignment = GetMemoryAlignment();                                           \
        return MCore::AlignedAllocate(numBytes, alignment, category, 0, MCORE_FILE, MCORE_LINE); \
    }                                                                                            \
                                                                                                 \
    void* operator new(size_t numBytes, void* location)                                          \
    {                                                                                            \
        static_cast<void>(numBytes);                                                             \
        return location;                                                                         \
    }                                                                                            \
                                                                                                 \
    void operator delete(void* memLocation)                                                      \
    {                                                                                            \
        MCore::AlignedFree(memLocation);                                                         \
    }                                                                                            \
                                                                                                 \
    void operator delete(void* memLocation, void* placement)                                     \
    {                                                                                            \
        static_cast<void>(memLocation);                                                          \
        static_cast<void>(placement);                                                            \
    }                                                                                            \
                                                                                                 \
    void* operator new[](size_t numBytes)                                                        \
    {                                                                                            \
        const uint16 category  = GetMemoryCategory();                                            \
        const uint16 alignment = GetMemoryAlignment();                                           \
        return MCore::AlignedAllocate(numBytes, alignment, category, 0, MCORE_FILE, MCORE_LINE); \
    }                                                                                            \
                                                                                                 \
    void* operator new[](size_t numBytes, void* place)                                           \
    {                                                                                            \
        static_cast<void>(numBytes);                                                             \
        return place;                                                                            \
    }                                                                                            \
                                                                                                 \
    void operator delete[](void* memLocation)                                                    \
    {                                                                                            \
        MCore::AlignedFree(memLocation);                                                         \
    }


    /**
     * Specify the memory category for a given class.
     * @param CLASSNAME The class name, which would be "FooBar" in case you declare your class as "class FooBar { .... };".
     * @param CATEGORY An integer containing the memory category ID where this class belongs to.
     */
    #define MCORE_MEMORYCATEGORY(CLASSNAME, CATEGORY) \
    uint32 CLASSNAME::GetMemoryCategory() { return CATEGORY; }

#else   // when we want no memory manager

    #define MCORE_MEMORYOBJECT(CLASSNAME, ALIGNMENT)
    #define MCORE_MEMORYOBJECTCATEGORY(CLASSNAME, ALIGNMENT, CATEGORY)
    #define MCORE_MEMORYCATEGORY(CLASSNAME, CATEGORY)

#endif

    //-------------------------------------------------------------------------------

    /**
     * Copy memory from source to dest, like memcpy.
     * If the source and destination overlap, the behavior of MemCopy is undefined. Use MemMove to handle overlapping regions.
     * @param dest The destination address to copy the data to.
     * @param source The source address to copy the data from.
     * @param numBytes The number of bytes to copy over.
     * @result The destination memory address as passed as the first parameter 'dest'.
     */
    MCORE_INLINE void* MemCopy(void* dest, const void* source, size_t numBytes)
    {
        return memcpy(dest, source, numBytes);
    }

    /**
     * Copy over memory from a source to destination memory address.
     * If some regions of the source area and the destination overlap, it is ensured that the original source
     * bytes in the overlapping region are copied before being overwritten. When using MemCopy this isn't assured.
     * @param dest The destination memory address.
     * @param source The source memory address.
     * @param numBytes The number of bytes to copy over.
     * @result The pointer of the destination memory address (dest).
     */
    MCORE_INLINE void* MemMove(void* dest, const void* source, size_t numBytes)
    {
        return memmove(dest, source, numBytes);
    }

    /**
     * Fill a block of memory with a given value.
     * @param address The address to start filling.
     * @param value The value to fill the data with.
     * @param numBytes The number of bytes to fill.
     * @result The address as specified in the first parameter.
     */
    MCORE_INLINE void* MemSet(void* address, const uint32 value, size_t numBytes)
    {
        return memset(address, value, numBytes);
    }

    //--------------------------------------------------------------------------------

    /**
     * Allocate a new piece of memory using malloc.
     * @param numBytes The number of bytes to allocate.
     * @param categoryID [ignored] The memory category.
     * @param blockID [ignored] The memory block/heap/pool/arena ID where to add this allocation to.
     * @param filename [ignored] The file name string where the allocation happens, which is allowed to be nullptr.
     * @param lineNr [ignored] The line number in the given file, where the allocation happens.
     * @result The start memory address of the allocated data.
     */
    MCORE_API void* StandardAllocate(size_t numBytes, uint16 categoryID = MCORE_MEMCATEGORY_UNKNOWN, uint16 blockID = 0, const char* filename = nullptr, uint32 lineNr = 0);

    /**
     * Reallocate a piece of memory using realloc.
     * If the parameter 'memory' is nullptr, it will act like a call to Allocate.
     * @param memory The memory address to reallocate, where nullptr is allowed as well.
     * @param numBytes The number of bytes to allocate.
     * @param [ignored] categoryID The memory category.
     * @param [ignored] blockID The memory block/heap/pool/arena ID where to add this allocation to.
     * @param [ignored] filename The file name string where the allocation happens, which is allowed to be nullptr.
     * @param [ignored] lineNr The line number in the given file, where the allocation happens.
     * @result The start memory address of the allocated data.
     */
    MCORE_API void* StandardRealloc(void* memory, size_t numBytes, uint16 categoryID = MCORE_MEMCATEGORY_UNKNOWN, uint16 blockID = 0, const char* filename = nullptr, uint32 lineNr = 0);

    /**
     * Free previously allocated memory using 'free(...)'.
     * @param memory The memory address to free.
     */
    MCORE_API void StandardFree(void* memory);

    //--------------------------------------------------------------------------------

    /**
     * Allocate a new piece of memory.
     * @param numBytes The number of bytes to allocate.
     * @param categoryID The memory category.
     * @param blockID The memory block/heap/pool/arena ID where to add this allocation to.
     * @param filename The file name string where the allocation happens, which is allowed to be nullptr.
     * @param lineNr The line number in the given file, where the allocation happens.
     * @result The start memory address of the allocated data.
     */
    MCORE_API void* Allocate(size_t numBytes, uint16 categoryID = MCORE_MEMCATEGORY_UNKNOWN, uint16 blockID = 0, const char* filename = nullptr, uint32 lineNr = 0);

    /**
     * Reallocate a piece of memory.
     * If the parameter 'memory' is nullptr, it will act like a call to Allocate.
     * @param memory The memory address to reallocate, where nullptr is allowed as well.
     * @param numBytes The number of bytes to allocate.
     * @param categoryID The memory category.
     * @param blockID The memory block/heap/pool/arena ID where to add this allocation to.
     * @param filename The file name string where the allocation happens, which is allowed to be nullptr.
     * @param lineNr The line number in the given file, where the allocation happens.
     * @result The start memory address of the allocated data.
     */
    MCORE_API void* Realloc(void* memory, size_t numBytes, uint16 categoryID = MCORE_MEMCATEGORY_UNKNOWN, uint16 blockID = 0, const char* filename = nullptr, uint32 lineNr = 0);

    /**
     * Free previously allocated memory.
     * @param memory The memory address to free.
     */
    MCORE_API void Free(void* memory);

    /**
     * Allocate memory and make sure this memory address that is returned is aligned to a given number of bytes.
     * This also introduces some overhead, as we store the unaligned memory address also inside the allocated memory.
     * So basically we are allocating a bit more than needed.
     * Please keep in mind that the alignment value has to be a power of two.
     * Also keep in mind that in order to free the memory again, you have to use MCore::AlignedFree( returnedPointer ).
     * Internally this function calls MCore::Malloc.
     * @param numBytes The number of bytes to allocate. Please keep in mind that due to the overhead we allocate a bit more.
     * @param alignment The alignment in bytes, which must be a power of two. If for example this is a value of 16, the memory address returned will be a multiple of 16 bytes.
     * @param categoryID The memory category.
     * @param blockID The memory block/heap/pool/arena ID where to add this allocation to.
     * @param filename The file name string where the allocation happens, which is allowed to be nullptr.
     * @param lineNr The line number in the given file, where the allocation happens.
     * @see AlignedFree
     * @see AlignedRealloc
     */
    MCORE_API void* AlignedAllocate(size_t numBytes, uint16 alignment = MCORE_DEFAULT_ALIGNMENT, uint16 categoryID = MCORE_MEMCATEGORY_UNKNOWN, uint16 blockID = 0, const char* filename = nullptr, uint32 lineNr = 0);

    /**
     * Reallocate a given aligned memory location that was allocated using AlignedAllocate.
     * @param alignedAddress The aligned memory address which you want to change the size of. This MUST have been allocated using MCore::AllignedAllocate.
     * @param numBytes The number of bytes to allocate. Please keep in mind that due to the overhead we allocate a bit more.
     * @param prevNumBytes The number of bytes that was allocated before. When set to 0, the data stored before the realloc can be lost or corrupted.
     * @param alignment The alignment in bytes, which must be a power of two. If for example this is a value of 16, the memory address returned will be a multiple of 16 bytes.
     * @param categoryID The memory category.
     * @param blockID The memory block/heap/pool/arena ID where to add this allocation to.
     * @param filename The file name string where the allocation happens, which is allowed to be nullptr.
     * @param lineNr The line number in the given file, where the allocation happens.
     * @see AlignedAllocate
     * @see AlignedFree
     */
    MCORE_API void* AlignedRealloc(void* alignedAddress, size_t numBytes, size_t prevNumBytes = 0, uint16 alignment = MCORE_DEFAULT_ALIGNMENT, uint16 categoryID = MCORE_MEMCATEGORY_UNKNOWN, uint16 blockID = 0, const char* filename = nullptr, uint32 lineNr = 0);

    /**
     * Free the memory that was allocated using MCore::AlignedAllocate or MCore::AlignedRealloc.
     * @param alignedAddress The aligned memory address to free.
     * @see AlignedAllocate
     * @see AlignedRealloc
     */
    MCORE_API void AlignedFree(void* alignedAddress);
}   // namespace MCore
