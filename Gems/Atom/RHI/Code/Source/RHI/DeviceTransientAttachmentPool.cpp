/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceTransientAttachmentPool.h>
#include <Atom/RHI/AliasedAttachmentAllocator.h>

namespace AZ::RHI
{
    bool DeviceTransientAttachmentPool::NeedsTransientAttachmentPool(const TransientAttachmentPoolDescriptor& descriptor)
    {
        switch (descriptor.m_heapParameters.m_type)
        {
        case HeapAllocationStrategy::Fixed:
        {
            // Fixed strategy must declare a budget for at least one type of resource in order to use a transient attachment pool.
            return
                descriptor.m_bufferBudgetInBytes != 0 ||
                descriptor.m_imageBudgetInBytes != 0 ||
                descriptor.m_renderTargetBudgetInBytes != 0;
        }
        // Paging and Memory Hint strategy can work with a 0 budget.
        case HeapAllocationStrategy::Paging:
        case HeapAllocationStrategy::MemoryHint:
            return true;
        default:
            AZ_Assert(false, "Invalid heap allocation type %d", descriptor.m_heapParameters.m_type);
            return false;
        }
    }

    ResultCode DeviceTransientAttachmentPool::Init(Device& device, const TransientAttachmentPoolDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("DeviceTransientAttachmentPool", false, "DeviceTransientAttachmentPool is already initialized!");
                return ResultCode::InvalidOperation;
            }
        }

        if (!ValidateInitParameters(descriptor))
        {
            return ResultCode::InvalidArgument;
        }

        m_descriptor = descriptor;

        const ResultCode resultCode = InitInternal(device, descriptor);

        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
        }

        return resultCode;
    }

    void DeviceTransientAttachmentPool::Shutdown()
    {
        if (IsInitialized())
        {
            ShutdownInternal();
            DeviceObject::Shutdown();
        }
    }

    void DeviceTransientAttachmentPool::Begin(const TransientAttachmentPoolCompileFlags compileFlags, const TransientAttachmentStatistics::MemoryUsage* memoryHint)
    {
        m_compileFlags = compileFlags;
        m_statistics.m_heaps.clear();
        m_statistics.m_scopes.clear();
        m_statistics.m_reservedMemory = {};

        m_currentScope = nullptr;
        BeginInternal(compileFlags, memoryHint);
    }

    void DeviceTransientAttachmentPool::BeginScope(Scope& scopeBase)
    {
        m_currentScope = static_cast<Scope*>(&scopeBase);

        TransientAttachmentStatistics::Scope scope;
        scope.m_scopeId = scopeBase.GetId();
        scope.m_hardwareQueueClass = scopeBase.GetHardwareQueueClass();
        m_statistics.m_scopes.push_back(scope);
    }

    void DeviceTransientAttachmentPool::EndScope()
    {
        m_currentScope = nullptr;
    }

    void DeviceTransientAttachmentPool::End()
    {
        EndInternal();
    }

    const TransientAttachmentStatistics& DeviceTransientAttachmentPool::GetStatistics() const
    {
        return m_statistics;
    }

    const TransientAttachmentPoolDescriptor& DeviceTransientAttachmentPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    TransientAttachmentPoolCompileFlags DeviceTransientAttachmentPool::GetCompileFlags() const
    {
        return m_compileFlags;
    }

    void DeviceTransientAttachmentPool::CollectHeapStats(AliasedResourceTypeFlags typeMask, AZStd::span<const TransientAttachmentStatistics::Heap> heapStats)
    {
        // [GFX_TODO][ATOM-4162] Report the memory allocated stat correctly (or as close as possible) when the heap
        // supports multiple resource types. Right now we are assigning all the memory used to one resource type.
        for (const auto& heapStat : heapStats)
        {
            if (CheckBitsAny(typeMask, RHI::AliasedResourceTypeFlags::RenderTarget))
            {
                m_statistics.m_reservedMemory.m_rendertargetMemoryInBytes += heapStat.m_heapSize;
            }
            else if (CheckBitsAny(typeMask, RHI::AliasedResourceTypeFlags::Buffer))
            {
                m_statistics.m_reservedMemory.m_bufferMemoryInBytes += heapStat.m_heapSize;
            }
            else if (CheckBitsAny(typeMask, RHI::AliasedResourceTypeFlags::Image))
            {
                m_statistics.m_reservedMemory.m_imageMemoryInBytes += heapStat.m_heapSize;
            }
        }
    }

    bool DeviceTransientAttachmentPool::ValidateInitParameters([[maybe_unused]] const TransientAttachmentPoolDescriptor& descriptor)
    {
#if defined (AZ_RHI_ENABLE_VALIDATION)
        switch (descriptor.m_heapParameters.m_type)
        {
        case HeapAllocationStrategy::Fixed:
        {
            if (descriptor.m_bufferBudgetInBytes == 0 &&
                descriptor.m_imageBudgetInBytes == 0 &&
                descriptor.m_renderTargetBudgetInBytes == 0)
            {
                AZ_Assert(false, "Invalid budget for transient attachment pool when using a Fixed allocation strategy");
                return false;
            }
            break;
        }
        case HeapAllocationStrategy::Paging:
        {
            const auto& pagingParameters = descriptor.m_heapParameters.m_pagingParameters;
            if (pagingParameters.m_pageSizeInBytes == 0)
            {
                AZ_Assert(false, "Invalid page size %d when using a Paging allocation strategy", pagingParameters.m_pageSizeInBytes);
                return false;
            }

            if (descriptor.m_bufferBudgetInBytes && pagingParameters.m_pageSizeInBytes > descriptor.m_bufferBudgetInBytes)
            {
                AZ_Assert(false, "Page size %d is bigger than budget for buffers %d", pagingParameters.m_pageSizeInBytes, descriptor.m_bufferBudgetInBytes);
                return false;
            }

            if (descriptor.m_imageBudgetInBytes && pagingParameters.m_pageSizeInBytes > descriptor.m_imageBudgetInBytes)
            {
                AZ_Assert(false, "Page size %d is bigger than budget for images %d", pagingParameters.m_pageSizeInBytes, descriptor.m_imageBudgetInBytes);
                return false;
            }

            if (descriptor.m_renderTargetBudgetInBytes && pagingParameters.m_pageSizeInBytes > descriptor.m_renderTargetBudgetInBytes)
            {
                AZ_Assert(false, "Page size %d is bigger than budget for rendertargets %d", pagingParameters.m_pageSizeInBytes, descriptor.m_renderTargetBudgetInBytes);
                return false;
            }

            if (pagingParameters.m_initialAllocationPercentage &&
                descriptor.m_bufferBudgetInBytes == 0 &&
                descriptor.m_imageBudgetInBytes == 0 &&
                descriptor.m_renderTargetBudgetInBytes == 0)
            {
                AZ_Assert(false, "Invalid initial allocation percentage (%lf) when using a Paging allocation strategy", pagingParameters.m_initialAllocationPercentage);
                return false;
            }
            break;
        }
        case HeapAllocationStrategy::MemoryHint:
        {
            const auto& memoryHintParameters = descriptor.m_heapParameters.m_usageHintParameters;
            if (memoryHintParameters.m_heapSizeScaleFactor < 1.0f)
            {
                AZ_Assert(false, "Invalid heap size scale factor (%lf) when using a MemoryHint allocation strategy", memoryHintParameters.m_heapSizeScaleFactor);
                return false;
            }

            if (memoryHintParameters.m_maxHeapWastedPercentage < 0.f || memoryHintParameters.m_maxHeapWastedPercentage > 1.f)
            {
                AZ_Assert(false, "Invalid max heap wasted percentage (%lf) when using a MemoryHint allocation strategy", memoryHintParameters.m_maxHeapWastedPercentage);
                return false;
            }

            if ((memoryHintParameters.m_heapSizeScaleFactor - 1.0f) >= memoryHintParameters.m_maxHeapWastedPercentage)
            {
                AZ_Assert(false, "Heap scale factor (%lf) is bugger than max wasted percentage (%lf) when using a MemoryHint allocation strategy",
                    memoryHintParameters.m_heapSizeScaleFactor,
                    memoryHintParameters.m_maxHeapWastedPercentage);
                return false;
            }
            break;
        }
        default:
            AZ_Assert(false, "Invalid heap allocation strategy %d", descriptor.m_heapParameters.m_type);
            return false;
        }
#endif
        return true;
    }
}
