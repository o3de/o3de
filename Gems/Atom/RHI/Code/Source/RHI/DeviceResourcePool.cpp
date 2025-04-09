/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceResourcePool.h>
#include <Atom/RHI/ResourcePoolDatabase.h>
#include <Atom/RHI/DeviceResource.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

//#define ASSERT_UNNAMED_RESOURCE_POOLS

namespace AZ::RHI
{
    DeviceResourcePool::~DeviceResourcePool()
    {
        AZ_Assert(m_registry.empty(), "ResourceType pool was not properly shutdown.");
    }

    uint32_t DeviceResourcePool::GetResourceCount() const
    {
        AZStd::shared_lock<AZStd::shared_mutex> lock(m_registryMutex);
        return static_cast<uint32_t>(m_registry.size());
    }

    ResourcePoolResolver* DeviceResourcePool::GetResolver()
    {
        return m_resolver.get();
    }

    const ResourcePoolResolver* DeviceResourcePool::GetResolver() const
    {
        return m_resolver.get();
    }

    void DeviceResourcePool::SetResolver(AZStd::unique_ptr<ResourcePoolResolver>&& resolver)
    {
        AZ_Assert(!IsInitialized(), "Assigning a resolver after the pool has been initialized is not allowed.");

        m_resolver = AZStd::move(resolver);
    }

    bool DeviceResourcePool::ValidateIsRegistered(const DeviceResource* resource) const
    {
        if (Validation::IsEnabled())
        {
            if (!resource || resource->GetPool() != this)
            {
                AZ_Error("DeviceResourcePool", false, "'%s': DeviceResource is not registered on this pool.", GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    bool DeviceResourcePool::ValidateIsUnregistered(const DeviceResource* resource) const
    {
        if (Validation::IsEnabled())
        {
            if (!resource || resource->GetPool() != nullptr)
            {
                AZ_Error("DeviceResourcePool", false, "'%s': DeviceResource is null or registered on another pool.", GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    bool DeviceResourcePool::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized() == false)
            {
                AZ_Error("DeviceResourcePool", false, "DeviceResource pool is not initialized.");
                return false;
            }
        }

        return true;
    }

    bool DeviceResourcePool::ValidateNotProcessingFrame() const
    {
        if (Validation::IsEnabled())
        {
            if (m_isProcessingFrame)
            {
                AZ_Error("DeviceResourcePool", false, "'%s' Attempting an operation that is invalid when processing the frame.", GetName().GetCStr());
                return false;
            }
        }

        return true;
    }

    void DeviceResourcePool::Register(DeviceResource& resource)
    {
        resource.SetPool(this);

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
        m_registry.emplace(&resource);
    }

    void DeviceResourcePool::Unregister(DeviceResource& resource)
    {
        resource.SetPool(nullptr);

        AZStd::unique_lock<AZStd::shared_mutex> lock(m_registryMutex);
        m_registry.erase(&resource);
    }

    ResultCode DeviceResourcePool::Init(
        Device& device,
        const ResourcePoolDescriptor& descriptor,
        const PlatformMethod& platformInitMethod)
    {
#ifdef ASSERT_UNNAMED_RESOURCE_POOLS
        AZ_Assert(!GetName().IsEmpty(), "Unnamed DeviceResourcePool created");
#endif
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("DeviceResourcePool", false, "DeviceResourcePool '%s' is already initialized.", GetName().GetCStr());
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

    void DeviceResourcePool::Shutdown()
    {
        AZ_Assert(ValidateNotProcessingFrame(), "Shutting down a pool while the frame is processing is undefined behavior.");

        // Multiple shutdown is allowed for pools.
        if (IsInitialized())
        {
            GetDevice().GetResourcePoolDatabase().DetachPool(this);
            FrameEventBus::Handler::BusDisconnect();
            MemoryStatisticsEventBus::Handler::BusDisconnect();
            for (DeviceResource* resource : m_registry)
            {
                resource->SetPool(nullptr);
                ShutdownResourceInternal(*resource);
                resource->Shutdown();
            }
            ShutdownInternal();
            m_registry.clear();
            m_memoryUsage = {};
            m_resolver.reset();
            DeviceObject::Shutdown();
        }
    }

    ResultCode DeviceResourcePool::InitResource(DeviceResource* resource, const PlatformMethod& platformInitResourceMethod)
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

    void DeviceResourcePool::ShutdownResource(DeviceResource* resource)
    {
        // [GFX_TODO][bethelz][LY-83244]: Frame processing validation disabled. See Jira.
        if (ValidateIsInitialized() && ValidateIsRegistered(resource) /* && ValidateNotProcessingFrame() */)
        {
            Unregister(*resource);
            ShutdownResourceInternal(*resource);
        }
    }

    void DeviceResourcePool::ShutdownInternal() {}
    void DeviceResourcePool::ShutdownResourceInternal(DeviceResource&) {}

    const HeapMemoryUsage& DeviceResourcePool::GetHeapMemoryUsage(HeapMemoryLevel memoryType) const
    {
        return m_memoryUsage.GetHeapMemoryUsage(memoryType);
    }

    const PoolMemoryUsage& DeviceResourcePool::GetMemoryUsage() const
    {
        return m_memoryUsage;
    }

    void DeviceResourcePool::OnFrameBegin()
    {
        m_memoryUsage.m_transferPull = {};
        m_memoryUsage.m_transferPush = {};
    }

    void DeviceResourcePool::OnFrameCompile()
    {
        if (Validation::IsEnabled())
        {
            m_isProcessingFrame = true;
        }
    }

    void DeviceResourcePool::OnFrameEnd()
    {
        if (Validation::IsEnabled())
        {
            m_isProcessingFrame = false;
        }
    }

    void DeviceResourcePool::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
    {
        MemoryStatistics::Pool* poolStats = builder.BeginPool();

        if (builder.GetReportFlags() == MemoryStatisticsReportFlags::Detail)
        {
            ForEach<DeviceResource>([&builder](const DeviceResource& resource)
            {
                resource.ReportMemoryUsage(builder);
            });

            ComputeFragmentation();
        }

        poolStats->m_name = GetName();
        poolStats->m_memoryUsage = m_memoryUsage;
        builder.EndPool();
    }
}
