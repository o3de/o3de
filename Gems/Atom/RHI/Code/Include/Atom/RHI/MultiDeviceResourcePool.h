/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>
#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/MemoryStatisticsBus.h>
#include <Atom/RHI/MultiDeviceObject.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::RHI
{
    class CommandList;
    template <class DeviceResource, class DeviceResourcePool>
    class MultiDeviceResource;
    class MemoryStatisticsBuilder;

    //! A base class for multi-device resource pools. This class facilitates registration of multi-device resources into the pool,
    //! and allows iterating child resource instances. The classes for the device specific Resource and ResourcePool are provided
    //! via template arguments which are forwarded to the corresponding MultiDeviceObject base and MultiDeviceResource class.
    template <class DeviceResource, class DeviceResourcePool>
    class MultiDeviceResourcePool : public MultiDeviceObject<DeviceResourcePool>
    {
        friend class MultiDeviceResource<DeviceResource, DeviceResourcePool>;

    public:
        AZ_RTTI((MultiDeviceResourcePool, "{BAE5442C-A312-4133-AE80-1200753A7C3E}", DeviceResource, DeviceResourcePool), MultiDeviceObject<DeviceResourcePool>);
        virtual ~MultiDeviceResourcePool();

        //! Shuts down the pool. This method will shutdown all resources associated with the pool.
        virtual void Shutdown() override = 0;

        //! Loops through every resource matching the provided resource type (RTTI casting is used) and
        //! calls the provided callback method. Both methods are thread-safe with respect to
        //! other Init calls. A shared_mutex is used to guard the internal registry. This means
        //! that multiple iterations can be done without blocking each other, but a resource Init / Shutdown
        //! will serialize with this method.
        template<typename ResourceType>
        void ForEach(AZStd::function<void(ResourceType&)> callback);

        template<typename ResourceType>
        void ForEach(AZStd::function<void(const ResourceType&)> callback) const;

        //! Returns the number of resources in the pool.
        uint32_t GetResourceCount() const;

        //! Returns the resource pool descriptor.
        virtual const ResourcePoolDescriptor& GetDescriptor() const = 0;

    protected:
        MultiDeviceResourcePool() = default;

        //!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//!//
        // Middle Layer API - Used by specific RHI pools.

        //! A simple functor that returns a result code.
        using PlatformMethod = AZStd::function<RHI::ResultCode()>;

        //! Validates the pool for initialization, calls the provided init method (which wraps the platform-specific
        //! resource init call). If the platform init fails, the resource pool is shutdown and an error code is returned.
        ResultCode Init(MultiDevice::DeviceMask deviceMask, const PlatformMethod& initMethod);

        //! Validates the state of resource, calls the provided init method, and registers the resource
        //! with the pool. If validation or the internal platform init method fail, the resource is not
        //! registered and an error code is returned.
        ResultCode InitResource(MultiDeviceResource<DeviceResource, DeviceResourcePool>* resource, const PlatformMethod& initResourceMethod);

        //! Validates the resource is registered / unregistered with the pool,
        //! and that it not null. In non-release builds this will issue a warning.
        //! Non-release builds will branch and fail the call if validation fails,
        //! but this should be treated as a bug, because release will disable
        //! validation.
        bool ValidateIsRegistered(const MultiDeviceResource<DeviceResource, DeviceResourcePool>* resource) const;
        bool ValidateIsUnregistered(const MultiDeviceResource<DeviceResource, DeviceResourcePool>* resource) const;

        //! Validates that the resource pool is initialized and ready to service requests.
        bool ValidateIsInitialized() const;

    private:
        //! Shuts down an resource by releasing all backing resources. This happens implicitly
        //! if the resource is released. The resource is still valid after this call, and can be
        //! re-initialized safely on another pool.
        void ShutdownResource(MultiDeviceResource<DeviceResource, DeviceResourcePool>* resource);

        //! Registers an resource instance with the pool (explicit pool derivations will do this).
        void Register(MultiDeviceResource<DeviceResource, DeviceResourcePool>& resource);

        //! Unregisters an resource instance with the pool.
        void Unregister(MultiDeviceResource<DeviceResource, DeviceResourcePool>& resource);

        //! The registry of resources initialized on the pool, guarded by a shared_mutex.
        mutable AZStd::shared_mutex m_registryMutex;
        AZStd::unordered_set<MultiDeviceResource<DeviceResource, DeviceResourcePool>*> m_mdRegistry;
    };

    template<class DeviceResource, class DeviceResourcePool> template<typename ResourceType>
    void MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::ForEach(AZStd::function<void(ResourceType&)> callback)
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
        for (MultiDeviceResource<DeviceResource, DeviceResourcePool>* resourceBase : m_mdRegistry)
        {
            ResourceType* resourceType = azrtti_cast<ResourceType*>(resourceBase);
            if (resourceType)
            {
                callback(*resourceType);
            }
        }
    }

    template<class DeviceResource, class DeviceResourcePool> template<typename ResourceType>
    void MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::ForEach(AZStd::function<void(const ResourceType&)> callback) const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
        for (const MultiDeviceResource<DeviceResource, DeviceResourcePool>* resourceBase : m_mdRegistry)
        {
            const ResourceType* resourceType = azrtti_cast<const ResourceType*>(resourceBase);
            if (resourceType)
            {
                callback(*resourceType);
            }
        }
    }

    template <class DeviceResource, class DeviceResourcePool>
    MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::~MultiDeviceResourcePool()
    {
        AZ_Assert(m_mdRegistry.empty(), "ResourceType pool was not properly shutdown.");
    }

    template <class DeviceResource, class DeviceResourcePool>
    uint32_t MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::GetResourceCount() const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
        return static_cast<uint32_t>(m_mdRegistry.size());
    }

    template <class DeviceResource, class DeviceResourcePool>
    bool MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::ValidateIsRegistered(const MultiDeviceResource<DeviceResource, DeviceResourcePool> *resource) const
    {
        if (Validation::IsEnabled())
        {
            if (!resource || resource->GetPool() != this)
            {
                AZ_Error(
                    "MultiDeviceResourcePool", false, "'%s': MultiDeviceResource is not registered on this pool.", this->GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    template <class DeviceResource, class DeviceResourcePool>
    bool MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::ValidateIsUnregistered(const MultiDeviceResource<DeviceResource, DeviceResourcePool> *resource) const
    {
        if (Validation::IsEnabled())
        {
            if (!resource || resource->GetPool() != nullptr)
            {
                AZ_Error(
                    "MultiDeviceResourcePool",
                    false,
                    "'%s': MultiDeviceResource is null or registered on another pool.",
                    this->GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    template <class DeviceResource, class DeviceResourcePool>
    bool MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (this->IsInitialized() == false)
            {
                AZ_Error("MultiDeviceResourcePool", false, "MultiDeviceResource pool is not initialized.");
                return false;
            }
        }

        return true;
    }

    template <class DeviceResource, class DeviceResourcePool>
    void MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::Register(MultiDeviceResource<DeviceResource, DeviceResourcePool> &resource)
    {
        resource.SetPool(this);

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
        m_mdRegistry.emplace(&resource);
    }

    template <class DeviceResource, class DeviceResourcePool>
    void MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::Unregister(MultiDeviceResource<DeviceResource, DeviceResourcePool> &resource)
    {
        resource.SetPool(nullptr);

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
        m_mdRegistry.erase(&resource);
    }

    template <class DeviceResource, class DeviceResourcePool>
    ResultCode MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::Init(MultiDevice::DeviceMask deviceMask, const PlatformMethod& platformInitMethod)
    {
        if (Validation::IsEnabled())
        {
            if (this->IsInitialized())
            {
                AZ_Error("MultiDeviceResourcePool", false, "MultiDeviceResourcePool '%s' is already initialized.", this->GetName().GetCStr());
                return ResultCode::InvalidOperation;
            }
        }

        MultiDeviceObject<DeviceResourcePool>::Init(deviceMask);

        ResultCode resultCode = platformInitMethod();

        return resultCode;
    }

    template <class DeviceResource, class DeviceResourcePool>
    void MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::Shutdown()
    {
        // Multiple shutdown is allowed for pools.
        if (this->IsInitialized())
        {
            for (MultiDeviceResource<DeviceResource, DeviceResourcePool>* resource : m_mdRegistry)
            {
                resource->SetPool(nullptr);
                resource->Shutdown();
            }
            m_mdRegistry.clear();
            MultiDeviceObject<DeviceResourcePool>::Shutdown();
        }
    }

    template <class DeviceResource, class DeviceResourcePool>
    ResultCode MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::InitResource(MultiDeviceResource<DeviceResource, DeviceResourcePool> *resource, const PlatformMethod& platformInitResourceMethod)
    {
        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (!ValidateIsUnregistered(resource))
        {
            return ResultCode::InvalidArgument;
        }

        const ResultCode resultCode = platformInitResourceMethod();
        if (resultCode == ResultCode::Success)
        {
            resource->Init(this->GetDeviceMask());
            Register(*resource);
        }
        return resultCode;
    }

    template <class DeviceResource, class DeviceResourcePool>
    void MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>::ShutdownResource(MultiDeviceResource<DeviceResource, DeviceResourcePool> *resource)
    {
        if (ValidateIsInitialized() && ValidateIsRegistered(resource))
        {
            Unregister(*resource);
        }
    }
} // namespace AZ::RHI
