/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/IPC/SharedMemory.h>

#include <AzCore/std/algorithm.h>

#include <AzCore/std/parallel/spin_mutex.h>

namespace AZ
{
    namespace Internal
    {
        struct RingData
        {
            AZ::u32 m_readOffset;
            AZ::u32 m_writeOffset;
            AZ::u32 m_startOffset;
            AZ::u32 m_endOffset;
            AZ::u32 m_dataToRead;
            AZ::u8 m_pad[32 - sizeof(AZStd::spin_mutex)];
        };
    } // namespace Internal
} // namespace AZ



using namespace AZ;



#include <stdio.h>

//=========================================================================
// SharedMemory
// [4/27/2011]
//=========================================================================
SharedMemory::SharedMemory()
    : m_data(nullptr)
{
}

//=========================================================================
// ~SharedMemory
// [4/27/2011]
//=========================================================================
SharedMemory::~SharedMemory()
{
    UnMap();
    Close();
}

//=========================================================================
// Create
// [4/27/2011]
//=========================================================================
SharedMemory_Common::CreateResult
SharedMemory::Create(const char* name, unsigned int size, bool openIfCreated)
{
    AZ_Assert(name && strlen(name) > 1, "Invalid name!");
    AZ_Assert(size > 0, "Invalid buffer size!");
    if (IsReady())
    {
        return CreateFailed;
    }

    CreateResult result = Platform::Create(name, size, openIfCreated);

    if (result != CreatedExisting)
    {
        MemoryGuard l(*this);
        if (Map())
        {
            Clear();
            UnMap();
        }
        else
        {
            return CreateFailed;
        }
        return CreatedNew;
    }

    return CreatedExisting;
}

//=========================================================================
// Open
// [4/27/2011]
//=========================================================================
bool
SharedMemory::Open(const char* name)
{
    AZ_Assert(name && strlen(name) > 1, "Invalid name!");

    if (Platform::IsMapHandleValid() || m_globalMutex)
    {
        return false;
    }

    return Platform::Open(name);
}

//=========================================================================
// Close
// [4/27/2011]
//=========================================================================
void
SharedMemory::Close()
{
    UnMap();
    Platform::Close();
}

//=========================================================================
// Map
// [4/27/2011]
//=========================================================================
bool
SharedMemory::Map(AccessMode mode, unsigned int size)
{
    AZ_Assert(m_mappedBase == nullptr, "We already have data mapped");
    AZ_Assert(Platform::IsMapHandleValid(), "You must call Map() first!");

    bool result = Platform::Map(mode, size);

    if (result)
    {
        m_data = reinterpret_cast<char*>(m_mappedBase);
    }

    return result;
}

//=========================================================================
// UnMap
// [4/27/2011]
//=========================================================================
bool
SharedMemory::UnMap()
{
    if (m_mappedBase == nullptr)
    {
        return false;
    }
    if (!Platform::UnMap())
    {
        AZ_TracePrintf("AZSystem", "UnmapViewOfFile failed with error %d\n", GetLastError());
        return false;
    }
    m_mappedBase = nullptr;
    m_data = nullptr;
    m_dataSize = 0;
    return true;
}

//=========================================================================
// UnMap
// [4/27/2011]
//=========================================================================
void SharedMemory::lock()
{
    AZ_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
    Platform::lock();
}

//=========================================================================
// UnMap
// [4/27/2011]
//=========================================================================
bool SharedMemory::try_lock()
{
    AZ_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
    return Platform::try_lock();
}

//=========================================================================
// UnMap
// [4/27/2011]
//=========================================================================
void SharedMemory::unlock()
{
    AZ_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
    Platform::unlock();
}

bool SharedMemory::IsLockAbandoned()
{ 
    return Platform::IsLockAbandoned();
}

//=========================================================================
// Clear
// [4/19/2013]
//=========================================================================
void  SharedMemory::Clear()
{
    if (m_mappedBase != nullptr)
    {
        AZ_Warning("AZSystem", !Platform::IsWaitFailed(), "You are clearing the shared memory %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);
        memset(m_data, 0, m_dataSize);
    }
}

bool SharedMemory::CheckMappedBaseValid()
{
    // Check that m_mappedBase is valid, and clean up if it isn't
    if (m_mappedBase == nullptr)
    {
        AZ_TracePrintf("AZSystem", "MapViewOfFile failed with error %d\n", GetLastError());
        Close();
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Shared Memory ring buffer
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// SharedMemoryRingBuffer
// [4/29/2011]
//=========================================================================
SharedMemoryRingBuffer::SharedMemoryRingBuffer()
    : m_info(nullptr)
{}

//=========================================================================
// Create
// [4/29/2011]
//=========================================================================
bool
SharedMemoryRingBuffer::Create(const char* name, unsigned int size, bool openIfCreated)
{
    return SharedMemory::Create(name, size + sizeof(Internal::RingData), openIfCreated) != SharedMemory::CreateFailed;
}

//=========================================================================
// Map
// [4/28/2011]
//=========================================================================
bool
SharedMemoryRingBuffer::Map(AccessMode mode, unsigned int size)
{
    if (SharedMemory::Map(mode, size))
    {
        MemoryGuard l(*this);
        m_info = reinterpret_cast<Internal::RingData*>(m_data);
        m_data = m_info + 1;
        m_dataSize -= sizeof(Internal::RingData);
        if (m_info->m_endOffset == 0)  // if endOffset == 0 we have never set the info structure, do this only once.
        {
            m_info->m_startOffset = 0;
            m_info->m_readOffset = m_info->m_startOffset;
            m_info->m_writeOffset = m_info->m_startOffset;
            m_info->m_endOffset = m_info->m_startOffset + m_dataSize;
            m_info->m_dataToRead = 0;
        }
        return true;
    }

    return false;
}

//=========================================================================
// UnMap
// [4/28/2011]
//=========================================================================
bool
SharedMemoryRingBuffer::UnMap()
{
    m_info = nullptr;
    return SharedMemory::UnMap();
}

//=========================================================================
// Write
// [4/28/2011]
//=========================================================================
bool
SharedMemoryRingBuffer::Write(const void* data, unsigned int dataSize)
{
    AZ_Warning("AZSystem", !Platform::IsWaitFailed(), "You are writing the ring buffer %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);
    AZ_Assert(m_info != nullptr, "You need to Create and Map the buffer first!");
    if (m_info->m_writeOffset >= m_info->m_readOffset)
    {
        unsigned int freeSpace = m_dataSize - (m_info->m_writeOffset - m_info->m_readOffset);
        // if we are full or don't have enough space return false
        if (m_info->m_dataToRead == m_dataSize || freeSpace < dataSize)
        {
            return false;
        }
        unsigned int copy1MaxSize = m_info->m_endOffset - m_info->m_writeOffset;
        unsigned int dataToCopy1 = AZStd::GetMin(copy1MaxSize, dataSize);
        if (dataToCopy1)
        {
            memcpy(reinterpret_cast<char*>(m_data) + m_info->m_writeOffset, data, dataToCopy1);
        }
        unsigned int dataToCopy2 = dataSize - dataToCopy1;
        if (dataToCopy2)
        {
            memcpy(reinterpret_cast<char*>(m_data) + m_info->m_startOffset, reinterpret_cast<const char*>(data) + dataToCopy1, dataToCopy2);
            m_info->m_writeOffset = m_info->m_startOffset + dataToCopy2;
        }
        else
        {
            m_info->m_writeOffset += dataToCopy1;
        }
    }
    else
    {
        unsigned int freeSpace = m_info->m_readOffset - m_info->m_writeOffset;
        if (freeSpace < dataSize)
        {
            return false;
        }
        memcpy(reinterpret_cast<char*>(m_data) + m_info->m_writeOffset, data, dataSize);
        m_info->m_writeOffset += dataSize;
    }
    m_info->m_dataToRead += dataSize;

    return true;
}

//=========================================================================
// Read
// [4/28/2011]
//=========================================================================
unsigned int
SharedMemoryRingBuffer::Read(void* data, unsigned int maxDataSize)
{
    AZ_Warning("AZSystem", !Platform::IsWaitFailed(), "You are reading the ring buffer %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);

    if (m_info->m_dataToRead == 0)
    {
        return 0;
    }

    AZ_Assert(m_info != nullptr, "You need to Create and Map the buffer first!");
    unsigned int dataRead;
    if (m_info->m_writeOffset > m_info->m_readOffset)
    {
        unsigned int dataToRead = AZStd::GetMin(m_info->m_writeOffset - m_info->m_readOffset, maxDataSize);
        if (dataToRead)
        {
            memcpy(data, reinterpret_cast<char*>(m_data) + m_info->m_readOffset, dataToRead);
        }
        m_info->m_readOffset += dataToRead;
        dataRead = dataToRead;
    }
    else
    {
        unsigned int dataToRead1 = AZStd::GetMin(m_info->m_endOffset - m_info->m_readOffset, maxDataSize);
        if (dataToRead1)
        {
            maxDataSize -= dataToRead1;
            memcpy(data, reinterpret_cast<char*>(m_data) + m_info->m_readOffset, dataToRead1);
        }
        unsigned int dataToRead2 = AZStd::GetMin(m_info->m_writeOffset - m_info->m_startOffset, maxDataSize);
        if (dataToRead2)
        {
            memcpy(reinterpret_cast<char*>(data) + dataToRead1, reinterpret_cast<char*>(m_data) + m_info->m_startOffset, dataToRead2);
            m_info->m_readOffset = m_info->m_startOffset + dataToRead2;
        }
        else
        {
            m_info->m_readOffset += dataToRead1;
        }
        dataRead = dataToRead1 + dataToRead2;
    }
    m_info->m_dataToRead -= dataRead;

    return dataRead;
}

//=========================================================================
// Read
// [4/28/2011]
//=========================================================================
unsigned int
SharedMemoryRingBuffer::DataToRead() const
{
    return m_info ? m_info->m_dataToRead : 0;
}

//=========================================================================
// Read
// [4/28/2011]
//=========================================================================
unsigned int
SharedMemoryRingBuffer::MaxToWrite() const
{
    return m_info ? (m_dataSize - m_info->m_dataToRead) : 0;
}

//=========================================================================
// Clear
// [4/19/2013]
//=========================================================================
void
SharedMemoryRingBuffer::Clear()
{
    SharedMemory::Clear();
    if (m_info)
    {
        m_info->m_readOffset = m_info->m_startOffset;
        m_info->m_writeOffset = m_info->m_startOffset;
        m_info->m_dataToRead = 0;
    }
}
