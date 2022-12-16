/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>
#include <Atom/RHI/MultiDeviceObject.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace RHI
    {
        class CommandList;
        class Resource;

        /**
         * A base class for resource pools. This class facilitates registration of resources
         * into the pool, and allows iterating child resource instances.
         */
        class ResourcePool : public MultiDeviceObject
        {
            friend class Resource;

        public:
            AZ_RTTI(ResourcePool, "{9CC5C385-234C-49AE-AE10-D94747C11EBC}", MultiDeviceObject);
            virtual ~ResourcePool();

            /// Shuts down the pool. This method will shutdown all resources associated with the pool.
            void Shutdown() override final;

            /**
             * Loops through every resource matching the provided resource type (RTTI casting is used) and
             * calls the provided callback method. Both methods are thread-safe with respect to
             * other Init calls. A shared_mutex is used to guard the internal registry. This means
             * that multiple iterations can be done without blocking each other, but a resource Init / Shutdown
             * will serialize with this method.
             */
            template <typename ResourceType>
            void ForEach(AZStd::function<void(ResourceType&)> callback);

            template <typename ResourceType>
            void ForEach(AZStd::function<void(const ResourceType&)> callback) const;

            /// Returns the number of resources in the pool.
            uint32_t GetResourceCount() const;

            // TODO: MultiDevice!
            /// Returns the resource pool descriptor.
            virtual const ResourcePoolDescriptor& GetDescriptor() const = 0;

            // TODO: MultiDevice!
            /// Returns the memory used by this pool for a specific heap type.
            const HeapMemoryUsage& GetHeapMemoryUsage(HeapMemoryLevel heapMemoryLevel) const;

            /// Returns the memory used by this pool.
            const PoolMemoryUsage& GetMemoryUsage() const;

        protected:
            ResourcePool() = default;

            // Platform API - Used by Platform pool implementations.

            /**
             * Pool memory usage is held by the base class. It is exposed for public const access and
             * protected mutable access. The budget components are assigned by this class (those should not
             * be touched as they are passed from the user), but the usage components are managed by the
             * platform pool implementation. The platform components are atomic, which enables for lock-free
             * memory tracking.
             */
            PoolMemoryUsage m_memoryUsage;

            /// Called when the pool is shutting down.
            virtual void ShutdownInternal();

            /// Called when a resource is being shut down.
            virtual void ShutdownResourceInternal(Resource& resource);

            // Middle Layer API - Used by specific RHI pools.

            /// A simple functor that returns a result code.
            using PlatformMethod = AZStd::function<RHI::ResultCode()>;

            /**
             * Validates the pool for initialization, calls the provided init method (which wraps the platform-specific
             * resource init call). If the platform init fails, the resource pool is shutdown and an error code is returned.
             */
            ResultCode Init(DeviceMask deviceMask, const ResourcePoolDescriptor& descriptor, const PlatformMethod& initMethod);

            /**
             * Validates the state of resource, calls the provided init method, and registers the resource
             * with the pool. If validation or the internal platform init method fail, the resource is not
             * registered and an error code is returned.
             */
            ResultCode InitResource(Resource* resource, const PlatformMethod& initResourceMethod);

            /**
             * Validates the resource is registered / unregistered with the pool,
             * and that it not null. In non-release builds this will issue a warning.
             * Non-release builds will branch and fail the call if validation fails,
             * but this should be treated as a bug, because release will disable
             * validation.
             */
            bool ValidateIsRegistered(const Resource* resource) const;
            bool ValidateIsUnregistered(const Resource* resource) const;

            /// Validates that the resource pool is initialized and ready to service requests.
            bool ValidateIsInitialized() const;

            /// Validates that we are not in the frame processing phase.
            bool ValidateNotProcessingFrame() const;

        private:
            /**
             * Shuts down an resource by releasing all backing resources. This happens implicitly
             * if the resource is released. The resource is still valid after this call, and can be
             * re-initialized safely on another pool.
             */
            void ShutdownResource(Resource* resource);

            /// Registers an resource instance with the pool (explicit pool derivations will do this).
            void Register(Resource& resource);

            /// Unregisters an resource instance with the pool.
            void Unregister(Resource& resource);

            /// The registry of resources initialized on the pool, guarded by a shared_mutex.
            mutable AZStd::shared_mutex m_registryMutex;
            AZStd::unordered_set<Resource*> m_registry;
        };

        template<typename ResourceType>
        void ResourcePool::ForEach(AZStd::function<void(ResourceType&)> callback)
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
            for (Resource* resourceBase : m_registry)
            {
                ResourceType* resourceType = azrtti_cast<ResourceType*>(resourceBase);
                if (resourceType)
                {
                    callback(*resourceType);
                }
            }
        }

        template<typename ResourceType>
        void ResourcePool::ForEach(AZStd::function<void(const ResourceType&)> callback) const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
            for (const Resource* resourceBase : m_registry)
            {
                const ResourceType* resourceType = azrtti_cast<const ResourceType*>(resourceBase);
                if (resourceType)
                {
                    callback(*resourceType);
                }
            }
        }
    }
}
