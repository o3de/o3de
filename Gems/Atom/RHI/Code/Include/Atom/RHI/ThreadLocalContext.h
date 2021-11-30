
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/threadbus.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * This class is a container of thread local storage. It allows for multiple instances
         * of thread local storage to exist simultaneously (a property not possible with the 
         * thread_local modifier, which is really a thread global). The context tracks AZ thread
         * lifetime through a bus in order to clean up storage for exiting threads. The context
         * allows thread-safe iteration of all thread contexts, which is also a property not possible
         * with thread_local.
         *
         * Finally, thread_local is used to cache access to storage for the context. The container
         * performs best when there are lots of back-to-back calls to the same container. Interleaving
         * container accesses will perform less optimally.
         */
        template <typename Storage>
        class ThreadLocalContext
            : public AZStd::ThreadEventBus::Handler
        {
        public:
            using InitFunction = AZStd::function<void(Storage&)>;

            ThreadLocalContext(InitFunction initFunction = [] (Storage&) {});
            ~ThreadLocalContext();

            // No copying or moving allowed.
            ThreadLocalContext(ThreadLocalContext&&) = delete;
            ThreadLocalContext(const ThreadLocalContext&) = delete;

            /**
             * Assigns a function to call when a new storage instance is instantiated on a thread.
             */
            void SetInitFunction(InitFunction initFunction);

            /**
             * Looks for a storage instance associated to the calling thread. If none
             * is found, a new storage instance is created and added to the internal map associated
             * to the current thread. Subsequent accesses on the same thread will return the existing storage.
             *
             * The pointer is cached in thread_local storage, along with the container id.
             * If the id matches this container, the cached storage is valid and returned. This
             * makes consecutive accesses by the same container free of contention.
             */
            Storage& GetStorage();

            /**
             * Takes a shared lock on the container and iterates all thread storages using the provided visitor.
             */
            void ForEach(AZStd::function<void(Storage&)> visitor);
            void ForEach(AZStd::function<void(const Storage&)> visitor) const;

            /**
             * Clears all thread storage from the container.
             */
            void Clear();

        private:
            static uint32_t MakeId();

            ///////////////////////////////////////////////////////////////////////
            // ThreadBus::Handler
            void OnThreadEnter(const AZStd::thread::id&, const AZStd::thread_desc*) override {}
            void OnThreadExit(const AZStd::thread_id& id) override;
            ///////////////////////////////////////////////////////////////////////

            uint32_t m_id = 0;
            InitFunction m_initFunction;
            mutable AZStd::shared_mutex m_sharedMutex;
            AZStd::vector<AZStd::thread_id> m_threadIdList;
            AZStd::vector<AZStd::unique_ptr<Storage>> m_storageList;
        };

        template <typename Storage>
        ThreadLocalContext<Storage>::ThreadLocalContext(InitFunction initFunction)
            : m_id{MakeId()}
            , m_initFunction{initFunction}
        {
            AZStd::ThreadEventBus::Handler::BusConnect();
        }

        template <typename Storage>
        ThreadLocalContext<Storage>::~ThreadLocalContext()
        {
            AZStd::ThreadEventBus::Handler::BusDisconnect();
        }

        template <typename Storage>
        void ThreadLocalContext<Storage>::SetInitFunction(InitFunction initFunction)
        {
            m_initFunction = initFunction;
        }

        template <typename Storage>
        Storage& ThreadLocalContext<Storage>::GetStorage()
        {
            /**
             * Thread locals are used to cache the result on this thread. This allow
             * subsequent accesses to the same storage to have zero contention. The
             * ID is used to identify this class instance.
             */
            thread_local uint32_t s_containerId = static_cast<uint32_t>(-1);
            thread_local Storage* s_storage = nullptr;

            if (m_id != s_containerId)
            {
                AZStd::thread_id threadId = AZStd::this_thread::get_id();

                // First attempt to find the storage in the vector by taking a shared lock.
                {
                    AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);

                    size_t index = 0;
                    for (; index < m_threadIdList.size(); ++index)
                    {
                        if (m_threadIdList[index] == threadId)
                        {
                            break;
                        }
                    }

                    if (index != m_threadIdList.size())
                    {
                        // Cache the pointer (and unique id) to thread local storage.
                        Storage& storage = *m_storageList[index];
                        s_storage = &storage;
                        s_containerId = m_id;
                        return storage;
                    }
                }

                // Next, take a unique lock and add a new storage to the map.
                {
                    AZStd::unique_lock<AZStd::shared_mutex> lock(m_sharedMutex);
                    m_threadIdList.emplace_back(threadId);
                    m_storageList.emplace_back(AZStd::make_unique<Storage>());

                    // Cache the pointer (and unique id) to thread local storage.
                    Storage& storage = *m_storageList.back();
                    m_initFunction(storage);
                    s_storage = &storage;
                    s_containerId = m_id;
                    return storage;
                }
            }

            // The thread-local storage is valid.
            return *s_storage;
        }

        template <typename Storage>
        void ThreadLocalContext<Storage>::ForEach(AZStd::function<void(Storage&)> visitor)
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
            for (AZStd::unique_ptr<Storage>& storage : m_storageList)
            {
                visitor(*storage);
            }
        }

        template <typename Storage>
        void ThreadLocalContext<Storage>::ForEach(AZStd::function<void(const Storage&)> visitor) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_sharedMutex);
            for (const AZStd::unique_ptr<Storage>& storage : m_storageList)
            {
                visitor(*storage);
            }
        }

        template <typename Storage>
        void ThreadLocalContext<Storage>::Clear()
        {
            AZStd::unique_lock<AZStd::shared_mutex> lock(m_sharedMutex);
            m_threadIdList.clear();
            m_storageList.clear();

            // Reset the id to invalidate any active thread contexts.
            m_id = MakeId();
        }

        template <typename Storage>
        uint32_t ThreadLocalContext<Storage>::MakeId()
        {
            static EnvironmentVariable<AZStd::atomic_uint> counter = nullptr; // this counter is per-process

            if (!counter)
            {
                counter = Environment::CreateVariable<AZStd::atomic_uint>(AZ_CRC("ThreadContextMapIdMonotonicCounter", 0xb3e370ec), 1);
            }

            return counter->fetch_add(1);
        }

        template <typename Storage>
        void ThreadLocalContext<Storage>::OnThreadExit(const AZStd::thread_id& id)
        {
            // A thread exited. Take a unique lock and release it from the map.
            AZStd::unique_lock<AZStd::shared_mutex> lock(m_sharedMutex);
            for (size_t i = 0; i < m_threadIdList.size(); ++i)
            {
                if (m_threadIdList[i] == id)
                {
                    AZStd::swap(m_threadIdList[i], m_threadIdList.back());
                    m_threadIdList.pop_back();

                    AZStd::swap(m_storageList[i], m_storageList.back());
                    m_storageList.pop_back();

                    return;
                }
            }
        }
    }
}
