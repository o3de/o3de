/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Object.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    namespace RHI
    {
        struct NullMutex
        {
            inline void lock() {}
            inline bool try_lock() { return true; }
            inline void unlock() {}
        };

        struct ObjectCollectorTraits
        {
            /// The type of object serviced by the object collector. This must either derive from Object
            /// or share its interface.
            using ObjectType = Object;

            /// The mutex used to guard the object collector data.
            using MutexType = NullMutex;
        };

        using ObjectCollectorNotifyFunction = AZStd::function<void()>;

        /**
         * Deferred-releases reference-counted objects at a specific latency. Example: Use to batch-release
         * objects that exist on the GPU timeline at the end of the frame after syncing the oldest GPU frame.
         */
        template <typename Traits = ObjectCollectorTraits>
        class ObjectCollector
        {
        public:
            using ObjectType = typename Traits::ObjectType;
            using ObjectPtrType = Ptr<ObjectType>;

            ObjectCollector() = default;
            ~ObjectCollector();

            /**
             * This method is called when the collection pass is releasing an object. The user
             * can provide their own "collector" function to do something other than release
             * (e.g. reuse from a pool).
             */
            using CollectFunction = AZStd::function<void(ObjectType&)>;

            struct Descriptor
            {
                /// Number of collect calls before an object is collected.
                size_t m_collectLatency = 0;

                /// The collector function called when an object is collected.
                CollectFunction m_collectFunction = nullptr;
            };

            void Init(const Descriptor& descriptor);
            void Shutdown();

            /// Queues a single pointer for collection.
            void QueueForCollect(ObjectPtrType object);

            /// Queues an array of objects for collection.
            void QueueForCollect(ObjectType* objects, size_t objectCount);

            /// Queues an array of pointers to the object for collection.
            void QueueForCollect(ObjectType** objects, size_t objectCount);

            /// Runs a collection cycle. All objects scheduled for collection (according
            /// to the collection latency) are provided to the collect function (if it exists),
            /// and the references are released.
            void Collect(bool forceFlush = false);

            /// Returns the number of objects pending to be collected.
            /// Must not be called at collection time.
            size_t GetObjectCount() const;

            /// Notifies after the current set of pending objects is released.
            void Notify(ObjectCollectorNotifyFunction notifyFunction);

        private:
            void QueueForCollectInternal(ObjectPtrType object);

            struct Garbage
            {
                AZStd::vector<ObjectPtrType> m_objects;
                uint64_t m_collectIteration;
                AZStd::vector<ObjectCollectorNotifyFunction> m_notifies;
            };

            inline bool IsGarbageReady(size_t collectIteration)
            {
                return m_currentIteration - collectIteration >= m_descriptor.m_collectLatency;
            }

            Descriptor m_descriptor;

            size_t m_currentIteration = 0;

            mutable typename Traits::MutexType m_mutex;
            AZStd::vector<ObjectPtrType> m_pendingObjects;
            AZStd::vector<Garbage> m_pendingGarbage;
            AZStd::vector<ObjectCollectorNotifyFunction> m_pendingNotifies;
        };

        template <typename Traits>
        ObjectCollector<Traits>::~ObjectCollector()
        {
            AZ_Assert(m_pendingGarbage.empty(), "There is garbage that wasn't collected");
        }

        template <typename Traits>
        void ObjectCollector<Traits>::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
        }

        template <typename Traits>
        void ObjectCollector<Traits>::Shutdown()
        {
            Collect(true);
        }

        template <typename Traits>
        void ObjectCollector<Traits>::QueueForCollect(ObjectPtrType object)
        {
            m_mutex.lock();
            QueueForCollectInternal(AZStd::move(object));
            m_mutex.unlock();
        }

        template <typename Traits>
        void ObjectCollector<Traits>::QueueForCollect(ObjectType* objects, size_t objectCount)
        {
            m_mutex.lock();
            for (size_t i = 0; i < objectCount; ++i)
            {
                QueueForCollectInternal(&objects[i]);
            }
            m_mutex.unlock();
        }

        template <typename Traits>
        void ObjectCollector<Traits>::QueueForCollect(ObjectType** objects, size_t objectCount)
        {
            m_mutex.lock();
            for (size_t i = 0; i < objectCount; ++i)
            {
                QueueForCollectInternal(static_cast<ObjectType*>(objects[i]));
            }
            m_mutex.unlock();
        }

        template <typename Traits>
        void ObjectCollector<Traits>::QueueForCollectInternal(ObjectPtrType object)
        {
            AZ_Assert(object, "Queued a null object");
            m_pendingObjects.emplace_back(AZStd::move(object));
        }

        template <typename Traits>
        void ObjectCollector<Traits>::Collect(bool forceFlush)
        {
            AZ_PROFILE_SCOPE(RHI, "ObjectCollector: Collect");
            m_mutex.lock();
            if (m_pendingObjects.size())
            {
                m_pendingGarbage.push_back({ AZStd::move(m_pendingObjects), m_currentIteration });
            }

            if (!m_pendingNotifies.empty())
            {
                if (!m_pendingGarbage.empty())
                {
                    // find the newest garbage entry and add any pending notifies
                    Garbage& latestGarbage = m_pendingGarbage.front();
                    size_t latestGarbageAge = m_currentIteration - latestGarbage.m_collectIteration;

                    // check the rest of the entries to see if they are newer
                    for (size_t i = 1; i < m_pendingGarbage.size(); ++i)
                    {
                        size_t age = m_currentIteration - m_pendingGarbage[i].m_collectIteration;
                        if (age < latestGarbageAge)
                        {
                            latestGarbage = m_pendingGarbage[i];
                            latestGarbageAge = age;
                        }
                    }

                    latestGarbage.m_notifies.insert(latestGarbage.m_notifies.end(), m_pendingNotifies.begin(), m_pendingNotifies.end());
                }
                else
                {
                    // garbage queue is empty, notify now
                    for (auto& notifyFunction : m_pendingNotifies)
                    {
                        notifyFunction();
                    }
                }

                m_pendingNotifies.clear();
            }
            m_mutex.unlock();

            size_t objectCount = 0;
            size_t i = 0;
            while (i < m_pendingGarbage.size())
            {
                Garbage& garbage = m_pendingGarbage[i];
                if (IsGarbageReady(garbage.m_collectIteration) || forceFlush)
                {
                    if (m_descriptor.m_collectFunction)
                    {
                        for (ObjectPtrType& object : garbage.m_objects)
                        {
                            m_descriptor.m_collectFunction(*object);
                        }
                    }
                    objectCount += garbage.m_objects.size();

                    for (auto& notifyFunction : garbage.m_notifies)
                    {
                        notifyFunction();
                    }

                    garbage = AZStd::move(m_pendingGarbage.back());
                    m_pendingGarbage.pop_back();
                }
                else
                {
                    ++i;
                }
            }
            m_currentIteration++;
        }

        template <typename Traits>
        size_t ObjectCollector<Traits>::GetObjectCount() const
        {
            size_t objectCount = 0;
            m_mutex.lock();
            objectCount += m_pendingObjects.size();
            m_mutex.unlock();

            for (const Garbage& garbage : m_pendingGarbage)
            {
                objectCount += garbage.m_objects.size();
            }

            return objectCount;
        }

        template <typename Traits>
        void ObjectCollector<Traits>::Notify(ObjectCollectorNotifyFunction notifyFunction)
        {
            m_mutex.lock();
            m_pendingNotifies.push_back(notifyFunction);
            m_mutex.unlock();
        }
    }
}
