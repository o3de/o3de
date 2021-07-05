/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_SHARED_MEMORY_H
#define AZCORE_SHARED_MEMORY_H

#include <AzCore/base.h>
#include <AzCore/std/parallel/lock.h>

#include <AzCore/IPC/SharedMemory_Common.h>
#include <AzCore/IPC/SharedMemory_Platform.h>

namespace AZ
{
    namespace Internal
    {
        struct ControlData;
        struct RingData;
    }

    /**
    * Shared memory is a helper class to provide shared memory
    * for IPC (Inter Process Communication). Technically this is
    * the fastest way to communicate between two process on the same
    * machine. Obviously for remote data exchange you will need to use
    * Sockets/DCOM/etc.
    */
    class SharedMemory : public SharedMemory_Platform
    {
    protected:
        using Platform = SharedMemory_Platform;

        void*                   m_data;

        SharedMemory(const SharedMemory&);
        SharedMemory& operator=(const SharedMemory&);
    public:
        typedef AZStd::lock_guard<SharedMemory> MemoryGuard;

        SharedMemory();
        ~SharedMemory();

        /// Create a shared memory block. If openIfCreated is false all memory will be cleared to 0.
        CreateResult Create(const char* name, unsigned int size, bool openIfCreated = false);
        /// Open an existing shared memory block. If the block doesn't exist we will return false otherwise true.
        bool Open(const char* name);
        bool IsReady() const            { return Platform::IsReady(); }
        void Close();

        /// Maps to the created map. If size == 0 it will map the whole memory.
        bool Map(AccessMode mode = ReadWrite, unsigned int size = 0);
        bool IsMapped() const { return m_mappedBase != nullptr; }
        bool UnMap();

        /// Naming is conforming with AZStd::lock_guard/unique_lock/etc.
        void lock();
        bool try_lock();
        void unlock();
        /**
         * This function will return correct result only when you own the lock (you are inside lock()/unlock(). Otherwise it will always return false.
         * IMPORTANT: It's recommended that you check after each call to lock if the mutex was abandoned and if so to reset shared memory
         * as the state is unknown. As usual there are exceptional cases it which this can be fine, this is why we don't do it automatically.
         */
        bool IsLockAbandoned();
        ///

        const char* GetName() const     { return m_name; }

        void* Data()                    { return m_data; }
        const void* Data() const        { return m_data; }
        unsigned int DataSize() const   { return m_dataSize; }
        /// Sets all data (if mapped to 0)
        void  Clear();

    private:
        bool CheckMappedBaseValid();

        friend SharedMemory_Platform;
    };

    /**
     * Shared memory with read and write pointers.
     */
    class SharedMemoryRingBuffer
        : public SharedMemory
    {
        Internal::RingData*     m_info;
        bool                    m_isSetup;

        SharedMemoryRingBuffer(const SharedMemoryRingBuffer& rhs);
        SharedMemoryRingBuffer& operator=(const SharedMemoryRingBuffer&);
    public:
        SharedMemoryRingBuffer();

        // If return true if we are create
        bool Create(const char* name, unsigned int size, bool openIfCreated = false);

        /// Maps to the created map. If size == 0 it will map the whole memory.
        bool Map(AccessMode mode = ReadWrite, unsigned int size = 0);
        bool UnMap();

        /// IMPORTANT: All functions below are UNSAFE. Don't forget to Lock/Unlock before/after using them.

        /// Returns true is we wrote the data, false if the free memory is insufficient.
        bool         Write(const void* data, unsigned int dataSize);
        /// Reads data up to the maxDataSize, returns number of bytes red.
        unsigned int Read(void* data, unsigned int maxDataSize);
        /// Get number of bytes to read.
        unsigned int DataToRead() const;
        /// Get maximum data we can write.
        unsigned int MaxToWrite() const;
        /// Clears the ring buffer data and reset it to initial condition.
        void  Clear();
    };
}

#endif // AZCORE_SHARED_MEMORY_H
#pragma once


