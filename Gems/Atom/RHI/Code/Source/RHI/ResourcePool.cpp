/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/ResourcePoolDatabase.h>

namespace AZ
{
    namespace RHI
    {
        ResourcePool::~ResourcePool()
        {
            AZ_Assert(m_registry.empty(), "ResourceType pool was not properly shutdown.");
        }

        uint32_t ResourcePool::GetResourceCount() const
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
            return static_cast<uint32_t>(m_registry.size());
        }

        bool ResourcePool::ValidateIsRegistered(const Resource* resource) const
        {
            if (Validation::IsEnabled())
            {
                if (!resource || resource->GetPool() != this)
                {
                    AZ_Error("ResourcePool", false, "'%s': Resource is not registered on this pool.", GetName().GetCStr());
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
                    AZ_Error("ResourcePool", false, "'%s': Resource is null or registered on another pool.", GetName().GetCStr());
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

        bool ResourcePool::ValidateNotProcessingFrame() const
        {
            return true;
        }

        void ResourcePool::Register(Resource& resource)
        {
            resource.SetPool(this);

            AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
            m_registry.emplace(&resource);
        }

        void ResourcePool::Unregister(Resource& resource)
        {
            resource.SetPool(nullptr);

            AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
            m_registry.erase(&resource);
        }

        ResultCode ResourcePool::Init(
            DeviceMask deviceMask, const ResourcePoolDescriptor& descriptor, const PlatformMethod& platformInitMethod)
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Error("ResourcePool", false, "ResourcePool '%s' is already initialized.", GetName().GetCStr());
                    return ResultCode::InvalidOperation;
                }
            }

            for (size_t heapMemoryLevel = 0; heapMemoryLevel < HeapMemoryLevelCount; ++heapMemoryLevel)
            {
                m_memoryUsage.m_memoryUsagePerLevel[heapMemoryLevel].m_budgetInBytes = descriptor.m_budgetInBytes;
            }

            MultiDeviceObject::Init(deviceMask);

            ResultCode resultCode = platformInitMethod();

            return resultCode;
        }

        void ResourcePool::Shutdown()
        {
            AZ_Assert(ValidateNotProcessingFrame(), "Shutting down a pool while the frame is processing is undefined behavior.");

            // Multiple shutdown is allowed for pools.
            if (IsInitialized())
            {
                for (Resource* resource : m_registry)
                {
                    resource->SetPool(nullptr);
                    ShutdownResourceInternal(*resource);
                    resource->Shutdown();
                }
                m_registry.clear();
                m_memoryUsage = {};
                ShutdownInternal();
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
            // [GFX_TODO][bethelz][LY-83244]: Frame processing validation disabled. See Jira.
            if (ValidateIsInitialized() && ValidateIsRegistered(resource) /* && ValidateNotProcessingFrame() */)
            {
                Unregister(*resource);
                ShutdownResourceInternal(*resource);
            }
        }

        void ResourcePool::ShutdownInternal()
        {
        }
        void ResourcePool::ShutdownResourceInternal(Resource&)
        {
        }

        const HeapMemoryUsage& ResourcePool::GetHeapMemoryUsage(HeapMemoryLevel memoryType) const
        {
            return m_memoryUsage.GetHeapMemoryUsage(memoryType);
        }

        const PoolMemoryUsage& ResourcePool::GetMemoryUsage() const
        {
            return m_memoryUsage;
        }

    } // namespace RHI
} // namespace AZ
