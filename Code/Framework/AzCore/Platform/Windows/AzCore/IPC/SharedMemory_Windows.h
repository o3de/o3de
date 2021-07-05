/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IPC/SharedMemory_Common.h>
#include <AzCore/PlatformIncl.h>

namespace AZ
{
    class SharedMemory_Windows : public SharedMemory_Common
    {
    protected:
        SharedMemory_Windows();

        bool IsReady() const
        {
            return m_mapHandle != NULL;
        }

        bool IsMapHandleValid() const
        {
            return m_mapHandle != nullptr;
        }

        CreateResult Create(const char* name, unsigned int size, bool openIfCreated);
        bool Open(const char* name);
        void Close();
        bool Map(AccessMode mode, unsigned int size);
        bool UnMap();
        void lock();
        bool try_lock();
        void unlock();
        bool IsLockAbandoned();
        bool IsWaitFailed() const;

        HANDLE m_mapHandle;
        HANDLE m_globalMutex;
        int m_lastLockResult;

    private:
        void ComposeMutexName(char* dest, size_t length, const char* name);
    };

    using SharedMemory_Platform = SharedMemory_Windows;
}
