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

#include <AzCore/IPC/SharedMemory_Common.h>
#include <semaphore.h>

namespace AZ
{
    class SharedMemory_Mac : public SharedMemory_Common
    {
    protected:
        SharedMemory_Mac();

        bool IsReady() const
        {
            return m_mapHandle != -1;
        }

        bool IsMapHandleValid() const
        {
            return m_mapHandle != -1;
        }

        static int GetLastError();

        void ComposeMutexName(char* dest, size_t length, const char* name);
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

        int m_mapHandle;
        sem_t* m_globalMutex;
    };

    using SharedMemory_Platform = SharedMemory_Mac;
}
