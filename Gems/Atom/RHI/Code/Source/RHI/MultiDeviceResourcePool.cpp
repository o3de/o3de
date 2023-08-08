/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/MultiDeviceResource.h>
#include <Atom/RHI/MultiDeviceResourcePool.h>
#include <Atom/RHI/ResourcePoolDatabase.h>

namespace AZ::RHI
{
    MultiDeviceResourcePool::~MultiDeviceResourcePool()
    {
        AZ_Assert(m_mdRegistry.empty(), "ResourceType pool was not properly shutdown.");
    }

    uint32_t MultiDeviceResourcePool::GetResourceCount() const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
        return static_cast<uint32_t>(m_mdRegistry.size());
    }

    bool MultiDeviceResourcePool::ValidateIsRegistered(const MultiDeviceResource* resource) const
    {
        if (Validation::IsEnabled())
        {
            if (!resource || resource->GetPool() != this)
            {
                AZ_Error(
                    "MultiDeviceResourcePool", false, "'%s': MultiDeviceResource is not registered on this pool.", GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    bool MultiDeviceResourcePool::ValidateIsUnregistered(const MultiDeviceResource* resource) const
    {
        if (Validation::IsEnabled())
        {
            if (!resource || resource->GetPool() != nullptr)
            {
                AZ_Error(
                    "MultiDeviceResourcePool",
                    false,
                    "'%s': MultiDeviceResource is null or registered on another pool.",
                    GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    bool MultiDeviceResourcePool::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized() == false)
            {
                AZ_Error("MultiDeviceResourcePool", false, "MultiDeviceResource pool is not initialized.");
                return false;
            }
        }

        return true;
    }

    void MultiDeviceResourcePool::Register(MultiDeviceResource& resource)
    {
        resource.SetPool(this);

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
        m_mdRegistry.emplace(&resource);
    }

    void MultiDeviceResourcePool::Unregister(MultiDeviceResource& resource)
    {
        resource.SetPool(nullptr);

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
        m_mdRegistry.erase(&resource);
    }

    ResultCode MultiDeviceResourcePool::Init(MultiDevice::DeviceMask deviceMask, const PlatformMethod& platformInitMethod)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("MultiDeviceResourcePool", false, "MultiDeviceResourcePool '%s' is already initialized.", GetName().GetCStr());
                return ResultCode::InvalidOperation;
            }
        }

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode = platformInitMethod();

        return resultCode;
    }

    void MultiDeviceResourcePool::Shutdown()
    {
        // Multiple shutdown is allowed for pools.
        if (IsInitialized())
        {
            for (MultiDeviceResource* resource : m_mdRegistry)
            {
                resource->SetPool(nullptr);
                resource->Shutdown();
            }
            m_mdRegistry.clear();
            MultiDeviceObject::Shutdown();
        }
    }

    ResultCode MultiDeviceResourcePool::InitResource(MultiDeviceResource* resource, const PlatformMethod& platformInitResourceMethod)
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

    void MultiDeviceResourcePool::ShutdownResource(MultiDeviceResource* resource)
    {
        if (ValidateIsInitialized() && ValidateIsRegistered(resource))
        {
            Unregister(*resource);
        }
    }
} // namespace AZ::RHI
