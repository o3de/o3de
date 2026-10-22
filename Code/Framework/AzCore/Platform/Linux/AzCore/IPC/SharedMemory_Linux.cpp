/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SharedMemory_Linux.h"
#include <AzCore/IPC/SharedMemory.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace AZ
{
    constexpr const char* MutexSuffix = "Mtx";
    constexpr const char* SharedMemorySuffix = "Data";
    constexpr const char* ShmMemoryLogName = "AZSystem";

    SharedMemory_Linux::SharedMemory_Linux() = default;

    [[nodiscard]] bool SharedMemory_Linux::IsReady() const
    {
        return m_mapHandle != -1;
    }

    [[nodiscard]] bool SharedMemory_Linux::IsMapHandleValid() const
    {
        return m_mapHandle != -1;
    }

    [[nodiscard]] bool SharedMemory_Linux::IsWaitFailed() const
    {
        return m_lastLockFailed;
    }

    bool SharedMemory_Linux::IsLockAbandoned()
    {
        // For now, we return false as this feature is not natively supported. (Same on MacOSX)
        return false;
    }

    int SharedMemory_Linux::GetLastError()
    {
        return errno;
    }

    static void ComposeName(SharedMemory_Linux::SharedMemoryPathType& dest, const char* name, const char* suffix)
    {
        dest = SharedMemory_Linux::SharedMemoryPathType::format("%s%s%s", "/", name, suffix);
    }

    SharedMemory_Common::CreateResult SharedMemory_Linux::Create(const char* name, unsigned int size, bool openIfCreated)
    {
        // Check if we do have a valid name.
        if (!name || name[0] == '\0')
        {
            AZ_Trace(ShmMemoryLogName, "Create called with invalid name\n");
            return CreateFailed;
        }

        azstrncpy(m_name, AZ_ARRAY_SIZE(m_name), name, strlen(name));

        // Create or open the named semaphore
        bool mutexCreated = false;
        SharedMemoryPathType fullName {};
        ComposeName(fullName, m_name, MutexSuffix);

        m_globalMutex = sem_open(fullName.c_str(), O_CREAT | O_EXCL, 0600, 1);
        if (m_globalMutex == SEM_FAILED)
        {
            if (errno != EEXIST)
            {
                AZ_Trace(ShmMemoryLogName, "Open new Mutex failed with error %d\n", errno);
                return CreateFailed;
            }

            // It exists already, try to open.
            m_globalMutex = sem_open(fullName.c_str(), 0);
            if (m_globalMutex == SEM_FAILED)
            {
                AZ_Trace(ShmMemoryLogName, "Open existing Mutex failed with error %d\n", errno);
                return CreateFailed;
            }
        }
        else
        {
            mutexCreated = true;
        }

        // Create or open the shared memory object
        ComposeName(fullName, m_name, SharedMemorySuffix);
        m_mapHandle = shm_open(fullName.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);

        const bool sharedMemoryCreated = (m_mapHandle != -1);
        if (!sharedMemoryCreated)
        {
            if (errno != EEXIST)
            {
                AZ_Trace(ShmMemoryLogName, "shm_open failed with error %d\n", errno);
                return CreateFailed;
            }

            if (!openIfCreated)
            {
                AZ_Trace(ShmMemoryLogName, "shm_open failed because it already exists and openIfCreated is false\n");
                return CreateFailed;
            }

            m_mapHandle = shm_open(fullName.c_str(), O_RDWR, 0600);
            if (m_mapHandle == -1)
            {
                AZ_Trace(ShmMemoryLogName, "shm_open failed with error %d\n", errno);
                return CreateFailed;
            }
        }

        if (sharedMemoryCreated)
        {
            while (flock(m_mapHandle, LOCK_EX) == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                AZ_Trace(ShmMemoryLogName, "flock failed with error %d\n", errno);
                shm_unlink(fullName.c_str());
                close(m_mapHandle);
                m_mapHandle = -1;
                return CreateFailed;
            }

            if (ftruncate(m_mapHandle, size) == -1)
            {
                AZ_Trace(ShmMemoryLogName, "ftruncate failed with error %d\n", errno);
                flock(m_mapHandle, LOCK_UN);
                shm_unlink(fullName.c_str());
                close(m_mapHandle);
                m_mapHandle = -1;
                return CreateFailed;
            }

            flock(m_mapHandle, LOCK_UN);
        }

        m_isCreator = sharedMemoryCreated;
        m_isCreatorShm = sharedMemoryCreated;
        m_isCreatorMutex = mutexCreated;

        return sharedMemoryCreated ? CreatedNew : CreatedExisting;
    }

    bool SharedMemory_Linux::Open(const char* name)
    {
        if (!name || name[0] == '\0')
        {
            AZ_Trace(ShmMemoryLogName, "Open called with invalid name\n");
            return false;
        }

        azstrncpy(m_name, AZ_ARRAY_SIZE(m_name), name, strlen(name));

        SharedMemoryPathType fullName;
        ComposeName(fullName, m_name, MutexSuffix);

        m_globalMutex = sem_open(fullName.c_str(), 0);
        if (m_globalMutex == SEM_FAILED)
        {
            AZ_Trace(ShmMemoryLogName, "sem_open %s failed with error %d\n", m_name, errno);
            m_globalMutex = nullptr;
            return false;
        }

        ComposeName(fullName, m_name, SharedMemorySuffix);
        m_mapHandle = shm_open(fullName.c_str(), O_RDWR, 0600);
        if (-1 == m_mapHandle)
        {
            AZ_Trace(ShmMemoryLogName, "shm_open %s failed with error %d\n", m_name, errno);
            sem_close(m_globalMutex);
            m_globalMutex = nullptr;
            return false;
        }

        m_isCreator = false;
        m_isCreatorShm = false;
        m_isCreatorMutex = false;

        return true;
    }

    void SharedMemory_Linux::Close()
    {
        if(m_isLocked)
        {
            unlock();
        }

        // Unmap if still mapped
        if (m_mappedBase && m_dataSize)
        {
            if (munmap(m_mappedBase, m_dataSize) == -1)
            {
                AZ_Trace(ShmMemoryLogName, "munmap failed with error %d\n", GetLastError());
            }
            m_mappedBase = nullptr;
            m_dataSize = 0;
        }

        if (-1 != m_mapHandle)
        {
            if (close(m_mapHandle) == -1)
            {
                AZ_Trace(ShmMemoryLogName, "close(shm fd) failed with error %d\n", GetLastError());
            }
        }

        if (m_globalMutex)
        {
            if (sem_close(m_globalMutex) == -1)
            {
                AZ_Trace(ShmMemoryLogName, "sem_close failed with error %d\n", GetLastError());
            }
        }

        // Unlink named objects only if we were the creator
        SharedMemoryPathType fullName;
        if(m_isCreatorShm)
        {
            ComposeName(fullName, m_name, SharedMemorySuffix);
            if (shm_unlink(fullName.c_str()) == -1 && errno != ENOENT)
            {
                AZ_Trace(ShmMemoryLogName, "shm_unlink failed with error %d\n", errno);
            }
        }

        if (m_isCreatorMutex)
        {
            ComposeName(fullName, m_name, MutexSuffix);
            if (sem_unlink(fullName.c_str()) == -1 && errno != ENOENT)
            {
                AZ_Trace(ShmMemoryLogName, "sem_unlink failed with error %d\n", errno);
            }
        }

        m_mapHandle = -1;
        m_globalMutex = nullptr;
        m_isCreator = false;
        m_isCreatorMutex = false;
        m_isCreatorShm = false;
        m_isLocked = false;
        m_lastLockFailed = false;
    }

    bool SharedMemory_Linux::Map(AccessMode mode, unsigned int size)
    {
        if (m_mapHandle == -1)
        {
            AZ_Trace(ShmMemoryLogName, "Map called with invalid handle\n");
            return false;
        }

        struct stat fileStat = { 0 };
        if (fstat(m_mapHandle, &fileStat) == -1)
        {
            AZ_Trace(ShmMemoryLogName, "fstat failed with error %d\n", GetLastError());
            return false;
        }

        if (size == 0)
        {
            size = static_cast<unsigned int>(fileStat.st_size);
        }

        // Safety check: refuse to map zero-byte shared memory. This can happen
        // if a second instance opens the object before the creator has called ftruncate,
        // or if the shm object was created but never properly sized.
        if (size == 0)
        {
            AZ_Trace(ShmMemoryLogName, "Map called with zero size\n");
            return false;
        }

        const int prot = (mode == ReadOnly ? PROT_READ : (PROT_READ | PROT_WRITE));
        void* base = mmap(nullptr, size, prot, MAP_SHARED, m_mapHandle, 0);
        if (base == MAP_FAILED)
        {
            AZ_Trace(ShmMemoryLogName, "mmap failed with error %d\n", GetLastError());
            return false;
        }

        m_mappedBase = base;
        m_dataSize = size;

        if (!static_cast<SharedMemory*>(this)->CheckMappedBaseValid())
        {
            if (munmap(m_mappedBase, m_dataSize) == -1)
            {
                AZ_Trace(ShmMemoryLogName, "munmap failed with error %d\n", GetLastError());
            }

            m_mappedBase = nullptr;
            m_dataSize = 0;
            return false;
        }

        return true;
    }

    bool SharedMemory_Linux::UnMap()
    {
        if (!m_mappedBase || m_dataSize == 0)
        {
            return true;
        }

        const int rc = munmap(m_mappedBase, m_dataSize);
        if (rc == 0)
        {
            m_mappedBase = nullptr;
            m_dataSize = 0;
            return true;
        }

        AZ_Trace(ShmMemoryLogName, "munmap failed with error %d\n", GetLastError());
        return false;
    }

    void SharedMemory_Linux::lock()
    {
        if (!m_globalMutex)
        {
            m_lastLockFailed = true;
            return;
        }

        m_lastLockFailed = false;
        m_isLocked = false;

        // errno is not guaranteed to be 0 on success, and may retain values from EINTR
        int rc;
        while ((rc = sem_wait(m_globalMutex)) == -1 && errno == EINTR) {}

        if (rc == -1)
        {
            AZ_Trace(ShmMemoryLogName, "sem_wait failed with error %d\n", GetLastError());
            m_lastLockFailed = true;
            return;
        }

        m_isLocked = true;
    }

    bool SharedMemory_Linux::try_lock()
    {
        if (!m_globalMutex)
        {
            return false;
        }

        const int rc = sem_trywait(m_globalMutex);
        if (rc == 0)
        {
            m_isLocked = true;
            m_lastLockFailed = false;
            return true;
        }

        if (errno == EAGAIN)
        {
            // Lock is held by another process - not a failure, just busy
            m_lastLockFailed = false;
            return false;
        }

        AZ_Trace(ShmMemoryLogName, "sem_trywait failed with error %d\n", GetLastError());
        m_lastLockFailed = true;
        return false;
    }

    void SharedMemory_Linux::unlock()
    {
        if (!m_globalMutex)
        {
            return;
        }

        if(!m_isLocked)
        {
            AZ_Trace(ShmMemoryLogName, "unlock() called without holding the lock\n");
            return;
        }

        if (sem_post(m_globalMutex) == -1)
        {
            AZ_Trace(ShmMemoryLogName, "sem_post failed with error %d\n", GetLastError());
        }
        m_isLocked = false;
    }
}
