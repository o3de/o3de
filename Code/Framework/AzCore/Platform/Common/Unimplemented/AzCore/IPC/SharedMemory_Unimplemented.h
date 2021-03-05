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

namespace AZ
{
    class SharedMemory_Unimplemented : public SharedMemory_Common
    {
    private:
        static void Unimplemented()
        {
            AZ_Assert(false, "SharedMemory not implemented on this platform!");
        }

    protected:
        static int GetLastError()
        {
            Unimplemented();
            return 0;
        }

        bool IsReady() const
        {
            Unimplemented();
            return false;
        }

        bool IsMapHandleValid() const
        {
            Unimplemented();
            return false;
        }

        CreateResult Create(const char* name, unsigned int size, bool openIfCreated)
        {
            AZ_UNUSED(name);
            AZ_UNUSED(size);
            AZ_UNUSED(openIfCreated);
            Unimplemented();
            return CreateResult::CreateFailed;
        }

        bool Open(const char* name)
        {
            AZ_UNUSED(name);
            Unimplemented();
            return false;
        }

        void Close()
        {
            Unimplemented();
        }

        bool Map(AccessMode mode, unsigned int size)
        {
            AZ_UNUSED(mode);
            AZ_UNUSED(size);
            Unimplemented();
            return false;
        }

        bool UnMap()
        {
            Unimplemented();
            return false;
        }

        void lock()
        {
            Unimplemented();
        }

        bool try_lock()
        {
            Unimplemented();
            return false;
        }

        void unlock()
        {
            Unimplemented();
        }

        bool IsLockAbandoned()
        {
            Unimplemented();
            return false;
        }

        bool IsWaitFailed() const
        {
            Unimplemented();
            return false;
        }

        void* m_globalMutex = nullptr;
    };

    using SharedMemory_Platform = SharedMemory_Unimplemented;
}
