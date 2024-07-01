/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/ResourcePoolDatabase.h>

namespace AZ::RHI
{
    ResourcePool::~ResourcePool()
    {
        AZ_Assert(m_Registry.empty(), "ResourceType pool was not properly shutdown.");
    }

    uint32_t ResourcePool::GetResourceCount() const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
        return static_cast<uint32_t>(m_Registry.size());
    }

    bool ResourcePool::ValidateIsRegistered(const Resource* resource) const
    {
        if (Validation::IsEnabled())
        {
            if (!resource || resource->GetPool() != this)
            {
                AZ_Error(
                    "ResourcePool", false, "'%s': Resource is not registered on this pool.", GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    bool ResourcePool::ValidateIsUnregistered(const Resource* resource) const
    {
        if (Validation::IsEnabled())
        {
            if (!resource || resource->GetPool() != nullptr)
            {
                AZ_Error(
                    "ResourcePool",
                    false,
                    "'%s': Resource is null or registered on another pool.",
                    GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    bool ResourcePool::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized() == false)
            {
                AZ_Error("ResourcePool", false, "Resource pool is not initialized.");
                return false;
            }
        }

        return true;
    }

    void ResourcePool::Register(Resource& resource)
    {
        resource.SetPool(this);

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
        m_Registry.emplace(&resource);
    }

    void ResourcePool::Unregister(Resource& resource)
    {
        resource.SetPool(nullptr);

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
        m_Registry.erase(&resource);
    }

    ResultCode ResourcePool::Init(MultiDevice::DeviceMask deviceMask, const PlatformMethod& platformInitMethod)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("ResourcePool", false, "ResourcePool '%s' is already initialized.", GetName().GetCStr());
                return ResultCode::InvalidOperation;
            }
        }

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode = platformInitMethod();

        return resultCode;
    }

    void ResourcePool::Shutdown()
    {
        // Multiple shutdown is allowed for pools.
        if (IsInitialized())
        {
            for (Resource* resource : m_Registry)
            {
                resource->SetPool(nullptr);
                resource->Shutdown();
            }
            m_Registry.clear();
            MultiDeviceObject::Shutdown();
        }
    }

    ResultCode ResourcePool::InitResource(Resource* resource, const PlatformMethod& platformInitResourceMethod)
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
            resource->Init(GetDeviceMask());
            Register(*resource);
        }
        return resultCode;
    }

    void ResourcePool::ShutdownResource(Resource* resource)
    {
        if (ValidateIsInitialized() && ValidateIsRegistered(resource))
        {
            Unregister(*resource);
        }
    }
} // namespace AZ::RHI
