/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/ObjectCollector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ::RHI
{
    //! Base class for managing creation / deletion of objects in the ObjectPool. When creating an object pool type,
    //! the user can specify a derived variant of RHI::Object, and the pool will internally manage objects
    //! using that factory.
    //! 
    //! Example Usage:
    //!      class CommandListFactory : public RHI::ObjectFactoryBase<CommandList>
    //!      {
    //!      public:
    //!          // Override descriptor struct with your own.
    //!          struct Descriptor
    //!          {
    //!              // Command queue type, device handle.
    //!          };
    //! 
    //!          void Init(const Descriptor& descriptor);
    //! 
    //!          // Only override the methods you care about.
    //!          CommandList* CreateObject();
    //! 
    //!      private:
    //!          Descriptor m_descriptor; // Store data here to inform CreateObject
    //!      };
    template <typename ObjectType>
    class ObjectFactoryBase
    {
    protected:
        ~ObjectFactoryBase() = default;

    public:
        struct Descriptor {};

        void Init(const Descriptor& descriptor) {}

        void Shutdown() {}

        /// Called when an object is being first created.
        template <typename... Args>
        Ptr<ObjectType> CreateObject(Args&&...)
        {
            return nullptr;
        }

        /// Called when a object collected object is being reset for new use.
        template <typename... Args>
        void ResetObject(ObjectType& object, Args&&...)
        {
            (void)object;
        }

        /// Called when the object is being shutdown.
        void ShutdownObject(ObjectType& object, bool isPoolShutdown)
        {
            (void)object;
            (void)isPoolShutdown;
        }

        /// Called when object collection has begun.
        void BeginCollect() {}

        /// Called when object collection has ended.
        void EndCollect() {}

        /// Called when the object is being object collected. Return true if the object should be recycled,
        /// or false if the object should be shutdown and released from the pool.
        bool CollectObject(ObjectType& object)
        {
            (void)object;
            return true;
        }
    };

    //! Base traits class for templatizing the ObjectPool. You can customize the object pool
    //! by overriding traits in this class.
    //!
    //! For example:
    //!      class MyObjectPoolTraits : public RHI::ObjectPoolTraits
    //!      {
    //!      public:
    //!          // Enable locking for thread-safe usage.
    //!          using MutexType = AZStd::mutex;
    //!
    //!          // Specify a type (the type must derive from Object or match its interface).
    //!          using ObjectType = MyObject;
    //!
    //!          // Specify a factory class which will manage creation / deletion of objects in the pool.
    //!          using ObjectFactoryType = MyObjectFactory;
    //!      };
    class ObjectPoolTraits
        : public ObjectCollectorTraits /// Inherits from object collector traits since they share types.
    {
    protected:
        ~ObjectPoolTraits() = default;

    public:
        /// The object factory class used to manage creation and deletion of objects from the pool.
        /// Override with your own type.
        using ObjectFactoryType = ObjectFactoryBase<ObjectType>;
    };

    //! This class is a simple deferred-release pool allocator for objects. It's useful when objects
    //! are being tracked on the GPU timeline, such that they require an N frame latency before being reused.
    //! For example: command lists which are being submitted to the GPU each frame. The derived type must inherit
    //! from Object.
    template <typename Traits>
    class ObjectPool
    {
    public:

        //! The type of object created and managed by this pool. Must inherit from Object.
        using ObjectType = typename Traits::ObjectType;

        //! The type of the object factory. An instance of this object
        using ObjectFactoryType = typename Traits::ObjectFactoryType;

        //! The type of the descriptor used to initialize the object factory.
        using ObjectFactoryDescriptor = typename ObjectFactoryType::Descriptor;

        //! The type of object collector used by the object pool.
        using ObjectCollectorType = ObjectCollector<Traits>;

        using MutexType = typename Traits::MutexType;

        ObjectPool() = default;
        ObjectPool(ObjectPool& rhs) = delete;

        struct Descriptor : public ObjectFactoryDescriptor
        {
            /// The number of GC iterations before objects in the pool will be recycled. Most useful when
            /// matched to the GPU / CPU fence latency.
            uint32_t m_collectLatency = 0;
        };

        //! Initializes the pool to an empty state.
        void Init(const Descriptor& descriptor)
        {
            m_factory.Init(descriptor);

            typename ObjectCollectorType::Descriptor collectorDesc;
            collectorDesc.m_collectLatency = descriptor.m_collectLatency;
            collectorDesc.m_collectFunction = [this](ObjectType& object)
            {
                if (m_isInitialized && m_factory.CollectObject(object))
                {
                    m_freeList.push(&object);
                }
                else
                {
                    m_factory.ShutdownObject(object, !m_isInitialized);
                    m_objects.erase(&object);
                }
            };

            m_collector.Init(collectorDesc);
            m_isInitialized = true;
        }

        //! Shutdown the pool. The user must re-initialize to use it again.
        void Shutdown()
        {
            m_isInitialized = false;
            m_collector.Shutdown();
            while (!m_freeList.empty())
            {
                m_freeList.pop();
            }
            for (AZStd::intrusive_ptr<ObjectType>& object : m_objects)
            {
                m_factory.ShutdownObject(*object, true);
            }
            m_objects.clear();
            m_factory.Shutdown();
        }

        //! Allocates an instance of an object from the pool. If no free object exists, it will
        //! create a new instance from the factory. If a free object exists, it will reuse that one.
        template <typename... Args>
        ObjectType* Allocate(Args&&... args)
        {
            ObjectType* objectForReset = nullptr;

            {
                AZStd::lock_guard<MutexType> lock(m_mutex);
                if (!m_freeList.empty())
                {
                    objectForReset = m_freeList.front();
                    m_freeList.pop();
                }
                else
                {
                    Ptr<ObjectType> objectPtr = m_factory.CreateObject(AZStd::forward<Args>(args)...);
                    if (objectPtr)
                    {
                        m_objects.emplace(objectPtr);
                    }
                    return objectPtr.get();
                }
            }

            if (objectForReset)
            {
                m_factory.ResetObject(*objectForReset, AZStd::forward<Args>(args)...);
            }

            return objectForReset;
        }

        //! Frees an object back to the pool. Depending on the object collection latency, it may take several
        //! cycles before the object is reused again.
        void DeAllocate(ObjectType* object)
        {
            m_collector.QueueForCollect(object);
        }

        void DeAllocate(ObjectType* objects, size_t objectCount)
        {
            m_collector.QueueForCollect(objects, objectCount);
        }

        void DeAllocate(ObjectType** objects, size_t objectCount)
        {
            m_collector.QueueForCollect(objects, objectCount);
        }

        //! Performs an object collection cycle. Objects which are collected can be reused by Allocate.
        void Collect()
        {
            m_factory.BeginCollect();
            m_collector.Collect();
            m_factory.EndCollect();
        }

        //! Performs an object collection cycle that ignores the collect latency, processing all objects.
        void CollectForce()
        {
            m_factory.BeginCollect();
            m_collector.CollectForce();
            m_factory.EndCollect();
        }

        //! Returns the total number of objects in the pool.
        size_t GetObjectCount() const
        {
            return m_objects.size();
        }

        const ObjectFactoryType& GetFactory() const
        {
            return m_factory;
        }

    private:
        ObjectFactoryType m_factory;
        ObjectCollector<Traits> m_collector;
        AZStd::unordered_set<Ptr<ObjectType>> m_objects;
        AZStd::queue<ObjectType*> m_freeList;
        MutexType m_mutex;
        bool m_isInitialized = false;
    };
}
