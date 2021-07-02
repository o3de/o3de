/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/ResourcePoolDatabase.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

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

        ResourcePoolResolver* ResourcePool::GetResolver()
        {
            return m_resolver.get();
        }

        const ResourcePoolResolver* ResourcePool::GetResolver() const
        {
            return m_resolver.get();
        }

        void ResourcePool::SetResolver(AZStd::unique_ptr<ResourcePoolResolver>&& resolver)
        {
            AZ_Assert(!IsInitialized(), "Assigning a resolver after the pool has been initialized is not allowed.");

            m_resolver = AZStd::move(resolver);
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
            if (Validation::IsEnabled())
            {
                if (m_isProcessingFrame)
                {
                    AZ_Error("ResourcePool", false, "'%s' Attempting an operation that is invalid when processing the frame.", GetName().GetCStr());
                    return false;
                }
            }

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
            Device& device,
            const ResourcePoolDescriptor& descriptor,
            const PlatformMethod& platformInitMethod)
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

            ResultCode resultCode = platformInitMethod();
            if (resultCode == ResultCode::Success)
            {
                DeviceObject::Init(device);
                MemoryStatisticsEventBus::Handler::BusConnect(&device);
                FrameEventBus::Handler::BusConnect(&device);
                device.GetResourcePoolDatabase().AttachPool(this);
            }
            return resultCode;
        }

        void ResourcePool::Shutdown()
        {
            AZ_Assert(ValidateNotProcessingFrame(), "Shutting down a pool while the frame is processing is undefined behavior.");

            // Multiple shutdown is allowed for pools.
            if (IsInitialized())
            {
                GetDevice().GetResourcePoolDatabase().DetachPool(this);
                FrameEventBus::Handler::BusDisconnect();
                MemoryStatisticsEventBus::Handler::BusDisconnect();
                for (Resource* resource : m_registry)
                {
                    resource->SetPool(nullptr);
                    ShutdownResourceInternal(*resource);
                    resource->Shutdown();
                }
                m_registry.clear();
                m_memoryUsage = {};
                m_resolver.reset();
                ShutdownInternal();
                DeviceObject::Shutdown();
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
                resource->Init(GetDevice());
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

        void ResourcePool::ShutdownInternal() {}
        void ResourcePool::ShutdownResourceInternal(Resource&) {}

        const HeapMemoryUsage& ResourcePool::GetHeapMemoryUsage(HeapMemoryLevel memoryType) const
        {
            return m_memoryUsage.GetHeapMemoryUsage(memoryType);
        }

        const PoolMemoryUsage& ResourcePool::GetMemoryUsage() const
        {
            return m_memoryUsage;
        }

        void ResourcePool::OnFrameBegin()
        {
            m_memoryUsage.m_transferPull = {};
            m_memoryUsage.m_transferPush = {};
        }

        void ResourcePool::OnFrameCompile()
        {
            if (Validation::IsEnabled())
            {
                m_isProcessingFrame = true;
            }
        }

        void ResourcePool::OnFrameEnd()
        {
            if (Validation::IsEnabled())
            {
                m_isProcessingFrame = false;
            }
        }

        void ResourcePool::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
        {
            MemoryStatistics::Pool* poolStats = builder.BeginPool();

            if (builder.GetReportFlags() == MemoryStatisticsReportFlags::Detail)
            {
                ForEach<Resource>([&builder](const Resource& resource)
                {
                    resource.ReportMemoryUsage(builder);
                });
            }

            poolStats->m_name = GetName();
            poolStats->m_memoryUsage = m_memoryUsage;
            builder.EndPool();
        }
    }
}
