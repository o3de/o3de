/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/MemoryStatisticsBus.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/parallel/shared_mutex.h>

namespace AZ::RHI
{
    class CommandList;
    class DeviceResource;
    class MemoryStatisticsBuilder;

    //! The resource pool resolver is a platform specific class for resolving a resource pool. Platforms
    //! control creation and usage of the resolver. Resolvers are queued onto scopes when a resolve is requested
    //! on a pool.
    class ResourcePoolResolver
    {
    public:
        AZ_RTTI(ResourcePoolResolver, "{2468216A-46EF-483C-9D0D-66F2EFC937BD}");
        virtual ~ResourcePoolResolver() = default;
    };

    //! A base class for resource pools. This class facilitates registration of resources
    //! into the pool, and allows iterating child resource instances.
    class DeviceResourcePool
        : public DeviceObject
        , public MemoryStatisticsEventBus::Handler
        , public FrameEventBus::Handler
    {
        friend class DeviceResource;
    public:
        AZ_RTTI(DeviceResourcePool, "{757EB674-25DC-4D00-9808-D3DAF33A4EFE}", DeviceObject);
        virtual ~DeviceResourcePool();

        //! Shuts down the pool. This method will shutdown all resources associated with the pool.
        void Shutdown() override final;

        //! Loops through every resource matching the provided resource type (RTTI casting is used) and
        //! calls the provided callback method. Both methods are thread-safe with respect to
        //! other Init calls. A shared_mutex is used to guard the internal registry. This means
        //! that multiple iterations can be done without blocking each other, but a resource Init / Shutdown
        //! will serialize with this method.
        template <typename ResourceType>
        void ForEach(AZStd::function<void(ResourceType&)> callback);

        template <typename ResourceType>
        void ForEach(AZStd::function<void(const ResourceType&)> callback) const;

        //! Returns the number of resources in the pool.
        uint32_t GetResourceCount() const;

        //! Returns the resolver for this pool.
        ResourcePoolResolver* GetResolver();
        const ResourcePoolResolver* GetResolver() const;

        //! Returns the resource pool descriptor.
        virtual const ResourcePoolDescriptor& GetDescriptor() const = 0;

        //! Returns the memory used by this pool for a specific heap type.
        const HeapMemoryUsage& GetHeapMemoryUsage(HeapMemoryLevel heapMemoryLevel) const;

        //! Returns the memory used by this pool.
        const PoolMemoryUsage& GetMemoryUsage() const;

    protected:
        DeviceResourcePool() = default;

        //////////////////////////////////////////////////////////////////////////
        // FrameEventBus::Handler
        void OnFrameBegin() override;
        void OnFrameCompile() override;
        void OnFrameEnd() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Platform API - Used by Platform pool implementations.

        //! Pool memory usage is held by the base class. It is exposed for public const access and
        //! protected mutable access. The budget components are assigned by this class (those should not
        //! be touched as they are passed from the user), but the usage components are managed by the
        //! platform pool implementation. The platform components are atomic, which enables for lock-free
        //! memory tracking.
        PoolMemoryUsage m_memoryUsage;

        //! Each platform implementation has the option to supply a resolver object. It's a platform
        //! defined class charged with performing resource data uploads on a scope in the FrameScheduler.
        //! Leaving this empty means the platform pool does not require a resolve operation.
        void SetResolver(AZStd::unique_ptr<ResourcePoolResolver>&& resolvePolicy);

        //! Called when the pool is shutting down.
        virtual void ShutdownInternal();

        //! Called when a resource is being shut down.
        virtual void ShutdownResourceInternal(DeviceResource& resource);

        //! Compute the memory fragmentation for each constituent heap and store the results in m_memoryUsage. This method is invoked
        //! when memory statistics gathering is active.
        virtual void ComputeFragmentation() const = 0;

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Middle Layer API - Used by specific RHI pools.

        /// A simple functor that returns a result code.
        using PlatformMethod = AZStd::function<RHI::ResultCode()>;

        //! Validates the pool for initialization, calls the provided init method (which wraps the platform-specific
        //! resource init call). If the platform init fails, the resource pool is shutdown and an error code is returned.
        ResultCode Init(
            Device& device,
            const ResourcePoolDescriptor& descriptor,
            const PlatformMethod& initMethod);

        //! Validates the state of resource, calls the provided init method, and registers the resource
        //! with the pool. If validation or the internal platform init method fail, the resource is not
        //! registered and an error code is returned.
        ResultCode InitResource(DeviceResource* resource, const PlatformMethod& initResourceMethod);

        //! Validates the resource is registered / unregistered with the pool,
        //! and that it not null. In non-release builds this will issue a warning.
        //! Non-release builds will branch and fail the call if validation fails,
        //! but this should be treated as a bug, because release will disable
        //! validation.
        bool ValidateIsRegistered(const DeviceResource* resource) const;
        bool ValidateIsUnregistered(const DeviceResource* resource) const;

        //! Validates that the resource pool is initialized and ready to service requests.
        bool ValidateIsInitialized() const;

        //! Validates that we are not in the frame processing phase.
        bool ValidateNotProcessingFrame() const;

    private:
        //////////////////////////////////////////////////////////////////////////
        // MemoryStatisticsEventBus::Handler
        void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const override final;
        //////////////////////////////////////////////////////////////////////////

        //! Shuts down an resource by releasing all backing resources. This happens implicitly
        //! if the resource is released. The resource is still valid after this call, and can be
        //! re-initialized safely on another pool.
        void ShutdownResource(DeviceResource* resource);

        //! Registers an resource instance with the pool (explicit pool derivations will do this).
        void Register(DeviceResource& resource);

        //! Unregisters an resource instance with the pool.
        void Unregister(DeviceResource& resource);

        //! The registry of resources initialized on the pool, guarded by a shared_mutex.
        mutable AZStd::shared_mutex m_registryMutex;
        AZStd::unordered_set<DeviceResource*> m_registry;

        //! The resolver is a policy object for handling a resolve operation (i.e. host to device data uploads). The
        //! derived class assigns this.
        AZStd::unique_ptr<ResourcePoolResolver> m_resolver;

        //! Tracks whether we are currently in a frame. Operations from the host which
        //! mutate GPU-accessible memory are not allowed within the frame. This enables
        //! the RHI pools to validate those operations.
        AZStd::atomic_bool m_isProcessingFrame = {false};
    };

    template <typename ResourceType>
    void DeviceResourcePool::ForEach(AZStd::function<void(ResourceType&)> callback)
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
        for (DeviceResource* resourceBase : m_registry)
        {
            ResourceType* resourceType = azrtti_cast<ResourceType*>(resourceBase);
            if (resourceType)
            {
                callback(*resourceType);
            }
        }
    }

    template <typename ResourceType>
    void DeviceResourcePool::ForEach(AZStd::function<void(const ResourceType&)> callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
        for (const DeviceResource* resourceBase : m_registry)
        {
            const ResourceType* resourceType = azrtti_cast<const ResourceType*>(resourceBase);
            if (resourceType)
            {
                callback(*resourceType);
            }
        }
    }
}
