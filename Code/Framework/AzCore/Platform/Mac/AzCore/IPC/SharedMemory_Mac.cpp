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
            if (openIfCreated)
            {
                m_globalMutex = sem_open(fullName, 0);
                error = errno;
            }
            else
            {
                sem_unlink(fullName);
                m_globalMutex = sem_open(fullName, O_CREAT | O_EXCL, 0600, 1);
                error = errno;
            }
        }
        if (m_globalMutex == SEM_FAILED)
        {
            m_globalMutex = nullptr;
            AZ_TracePrintf("AZSystem", "CreateMutex failed with error %d\n", error);
            return CreateFailed;
        }

        // Create the file mapping.
        ComposeName(fullName, AZ_ARRAY_SIZE(fullName), name, "Data");
        m_mapHandle = shm_open(fullName, O_RDWR | O_CREAT | O_EXCL, 0600);
        const bool createdNew = (m_mapHandle != -1);
        error = createdNew ? 0 : errno;
        if (!createdNew && error == EEXIST && openIfCreated)
        {
            m_mapHandle = shm_open(fullName, O_RDWR);
            error = (m_mapHandle == -1) ? errno : EEXIST;
        }
        if (m_mapHandle == -1)
        {
            AZ_TracePrintf("AZSystem", "CreateFileMapping failed with error %d\n", error);
            sem_close(m_globalMutex);
            m_globalMutex = nullptr;
            return CreateFailed;
        }
        if (createdNew && ftruncate(m_mapHandle, size) == -1)
        {
            AZ_TracePrintf("AZSystem", "ftruncate failed with error %d\n", errno);
            shm_unlink(fullName);
            close(m_mapHandle);
            m_mapHandle = -1;
            sem_close(m_globalMutex);
            m_globalMutex = nullptr;
            return CreateFailed;
        }

        return createdNew ? CreatedNew : CreatedExisting;
    }

    bool SharedMemory_Mac::Open(const char* name)
    {
        char fullName[256];
        azstrncpy(m_name, AZ_ARRAY_SIZE(m_name), name, strlen(name));
        ComposeName(fullName, AZ_ARRAY_SIZE(fullName), name, "Mtx");

        m_globalMutex = sem_open(fullName, 0);
        AZ_Warning("AZSystem", m_globalMutex != SEM_FAILED && m_globalMutex != nullptr, "Failed to open OS mutex [%s]\n", m_name);
        if (m_globalMutex == SEM_FAILED || m_globalMutex == nullptr)
        {
            m_globalMutex = nullptr;
            AZ_TracePrintf("AZSystem", "OpenMutex %s failed with error %d\n", m_name, errno);
            return false;
        }

        ComposeName(fullName, AZ_ARRAY_SIZE(fullName), name, "Data");
        m_mapHandle = shm_open(fullName, O_RDWR, 0600);
        if (m_mapHandle == -1)
        {
            AZ_TracePrintf("AZSystem", "OpenFileMapping %s failed with error %d\n", m_name, errno);
            sem_close(m_globalMutex);
            m_globalMutex = nullptr;
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

        if (m_globalMutex != nullptr && m_globalMutex != SEM_FAILED && sem_close(m_globalMutex) == -1)
        {
            AZ_TracePrintf("AZSystem", "CloseHandle failed with error %d\n", GetLastError());
        }

        m_mapHandle = -1;
        m_globalMutex = nullptr;
    }

    bool SharedMemory_Mac::Map(AccessMode mode, unsigned int size)
    {
        struct stat st;
        if (fstat(m_mapHandle, &st) == -1)
        {
            AZ_TracePrintf("AZSystem", "fstat failed with error %d\n", GetLastError());
            return false;
        }
        if (size == 0)
        {
            size = static_cast<unsigned int>(st.st_size);
        }
        if (size == 0)
        {
            return false;
        }
        int dwDesiredAccess = (mode == ReadOnly ? PROT_READ : PROT_READ | PROT_WRITE);
        m_mappedBase = mmap(nullptr, size, dwDesiredAccess, MAP_SHARED, m_mapHandle, 0);
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

        m_dataSize = size;

        return true;
    }

    bool SharedMemory_Mac::UnMap()
    {
        return munmap(m_mappedBase, m_dataSize) == 0;
    }

    void SharedMemory_Mac::lock()
    {
        if (m_globalMutex != nullptr && m_globalMutex != SEM_FAILED)
        {
            while (sem_wait(m_globalMutex) == -1 && errno == EINTR) {}
        }
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
