/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SharedMemory_Mac.h"
#include <AzCore/IPC/SharedMemory.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace AZ
{
    SharedMemory_Mac::SharedMemory_Mac() 
        : m_mapHandle(-1)
        , m_globalMutex(nullptr)
    {
    }

    int SharedMemory_Mac::GetLastError()
    {
        return errno;
    }

    void ComposeName(char* dest, size_t length, const char* name, const char* suffix)
    {
        // sem_open doesn't support paths bigger than 31, so call it s_Mtx.
        // While this is enough for Profiler, maybe the code should be smarter
        // and trim the name instead
        azsnprintf(dest, length, "/tmp/%s_%s", name, suffix);
    }

    SharedMemory_Common::CreateResult SharedMemory_Mac::Create(const char* name, unsigned int size, bool openIfCreated)
    {
        char fullName[256];
        azstrncpy(m_name, AZ_ARRAY_SIZE(m_name), name, strlen(name));
        ComposeName(fullName, AZ_ARRAY_SIZE(fullName), name, "Mtx");

        m_globalMutex = sem_open(fullName, O_CREAT | O_EXCL, 0600, 1);
        int error = errno;
        if (m_globalMutex == SEM_FAILED && (error == EEXIST))
        {
            sem_unlink(fullName);
            m_globalMutex = sem_open(fullName, O_CREAT | O_EXCL, 0600, 1);
            error = errno;
        }
        if (m_globalMutex == SEM_FAILED)
        {
            AZ_TracePrintf("AZSystem", "CreateMutex failed with error %d\n", error);
            return CreateFailed;
        }

        // Create the file mapping.
        ComposeName(fullName, AZ_ARRAY_SIZE(fullName), name, "Data");
        m_mapHandle = shm_open(fullName, O_RDWR | O_CREAT | O_EXCL, 0600);
        error = errno;
        if (error == EEXIST)
        {
            m_mapHandle = shm_open(fullName, O_RDWR);
            error = errno;
        }
        if (m_mapHandle == -1 || (error == EEXIST && openIfCreated == false))
        {
            AZ_TracePrintf("AZSystem", "CreateFileMapping failed with error %d\n", error);
            return CreateFailed;
        }
        ftruncate(m_mapHandle, size);

        return (error == EEXIST) ? CreatedExisting : CreatedNew;
    }

    bool SharedMemory_Mac::Open(const char* name)
    {
        char fullName[256];
        azstrncpy(m_name, AZ_ARRAY_SIZE(m_name), name, strlen(name));
        ComposeName(fullName, AZ_ARRAY_SIZE(fullName), name, "Mtx");

        m_globalMutex = sem_open(fullName, 0);
        AZ_Warning("AZSystem", m_globalMutex != nullptr, "Failed to open OS mutex [%s]\n", m_name);
        if (m_globalMutex == nullptr)
        {
            AZ_TracePrintf("AZSystem", "OpenMutex %s failed with error %d\n", m_name, errno);
            return false;
        }

        ComposeName(fullName, AZ_ARRAY_SIZE(fullName), name, "Data");
        m_mapHandle = shm_open(fullName, O_RDWR, 0600);
        if (m_mapHandle == -1)
        {
            AZ_TracePrintf("AZSystem", "OpenFileMapping %s failed with error %d\n", m_name, errno);
            return false;
        }

        return true;
    }

    void SharedMemory_Mac::Close()
    {
        if (m_mapHandle != -1 && close(m_mapHandle) == -1)
        {
            AZ_TracePrintf("AZSystem", "CloseHandle failed with error %d\n", GetLastError());
        }

        if (m_globalMutex != nullptr && sem_close(m_globalMutex) == -1)
        {
            AZ_TracePrintf("AZSystem", "CloseHandle failed with error %d\n", GetLastError());
        }

        m_mapHandle = -1;
        m_globalMutex = nullptr;
    }

    bool SharedMemory_Mac::Map(AccessMode mode, unsigned int size)
    {
        struct stat st;
        fstat(m_mapHandle, &st);
        if (size == 0)
        {
            size = static_cast<unsigned int>(st.st_size);
        }
        int dwDesiredAccess = (mode == ReadOnly ? PROT_READ : PROT_READ | PROT_WRITE);
        m_mappedBase = mmap(0, size, dwDesiredAccess, MAP_SHARED, m_mapHandle, 0);
        m_mappedBase = (m_mappedBase == MAP_FAILED) ? nullptr : m_mappedBase;

        if (m_mappedBase == nullptr)
        {
            AZ_TracePrintf("AZSystem", "MapViewOfFile failed with error %d\n", GetLastError());
            Close();
            return false;
        }

        if (!static_cast<SharedMemory*>(this)->CheckMappedBaseValid())
        {
            return false;
        }

        m_dataSize = static_cast<unsigned int>(st.st_size);

        return true;
    }

    bool SharedMemory_Mac::UnMap()
    {
        return munmap(m_mappedBase, m_dataSize) == 0;
    }

    void SharedMemory_Mac::lock()
    {
        sem_wait(m_globalMutex);
    }

    bool SharedMemory_Mac::try_lock()
    {
        return sem_trywait(m_globalMutex) == 0;
    }

    void SharedMemory_Mac::unlock()
    {
        sem_post(m_globalMutex);
    }

    bool SharedMemory_Mac::IsLockAbandoned()
    {
        return false;
    }

    bool SharedMemory_Mac::IsWaitFailed() const
    {
        return false;
    }

}
