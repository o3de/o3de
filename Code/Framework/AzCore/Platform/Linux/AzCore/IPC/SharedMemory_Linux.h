/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IPC/SharedMemory_Common.h>
#include <AzCore/std/string/string.h>
#include <semaphore.h>

namespace AZ
{
    class SharedMemory_Linux : public SharedMemory_Common
    {
    public:
        using SharedMemoryPathType = AZStd::fixed_string<AZ_ARRAY_SIZE(m_name)>;
    protected:
        SharedMemory_Linux();

        [[nodiscard]] bool IsReady() const;
        [[nodiscard]] bool IsMapHandleValid() const;
        bool IsLockAbandoned();
        [[nodiscard]] bool IsWaitFailed() const;
        static int GetLastError();

        CreateResult Create(const char* name, unsigned int size, bool openIfCreated);
        bool Open(const char* name);
        void Close();
        bool Map(AccessMode mode, unsigned int size);
        bool UnMap();
        void lock();
        bool try_lock();
        void unlock();

        sem_t* m_globalMutex {nullptr};
        int m_mapHandle {-1};
        bool m_isCreator{false};
        bool m_lastLockFailed{false};
        bool m_isLocked{false};
        bool m_isCreatorMutex {false};
        bool m_isCreatorShm {false};
    };

    using SharedMemory_Platform = SharedMemory_Linux;
}
