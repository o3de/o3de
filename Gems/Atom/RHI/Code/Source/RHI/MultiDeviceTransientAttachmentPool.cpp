/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/AliasedAttachmentAllocator.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceTransientAttachmentPool.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    bool MultiDeviceTransientAttachmentPool::NeedsTransientAttachmentPool(const TransientAttachmentPoolDescriptor& descriptor)
    {
        switch (descriptor.m_heapParameters.m_type)
        {
        case HeapAllocationStrategy::Fixed:
            {
                // Fixed strategy must declare a budget for at least one type of resource in order to use a transient attachment pool.
                return descriptor.m_bufferBudgetInBytes != 0 || descriptor.m_imageBudgetInBytes != 0 ||
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

    ResultCode MultiDeviceTransientAttachmentPool::Init(
        MultiDevice::DeviceMask deviceMask, const TransientAttachmentPoolDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("MultiDeviceTransientAttachmentPool", false, "MultiDeviceTransientAttachmentPool is already initialized!");
                return ResultCode::InvalidOperation;
            }
        }

        if (!ValidateInitParameters(descriptor))
        {
            return ResultCode::InvalidArgument;
        }

        m_descriptor = descriptor;

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode = ResultCode::Success;

        IterateDevices(
            [this, &descriptor, &resultCode](int deviceIndex)
            {
                auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                m_deviceTransientAttachmentPools[deviceIndex] = Factory::Get().CreateTransientAttachmentPool();

                TransientAttachmentPoolDescriptor deviceDescriptor;
                deviceDescriptor.m_bufferBudgetInBytes = descriptor.m_bufferBudgetInBytes;
                deviceDescriptor.m_heapParameters = descriptor.m_heapParameters;
                deviceDescriptor.m_imageBudgetInBytes = descriptor.m_imageBudgetInBytes;
                deviceDescriptor.m_renderTargetBudgetInBytes = descriptor.m_renderTargetBudgetInBytes;

                resultCode = m_deviceTransientAttachmentPools[deviceIndex]->Init(*device, deviceDescriptor);

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific TransientAttachmentPools and set deviceMask to 0
            m_deviceTransientAttachmentPools.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    void MultiDeviceTransientAttachmentPool::Shutdown()
    {
        if (IsInitialized())
        {
            for (const auto& [deviceIndex, pool] : m_deviceTransientAttachmentPools)
            {
                pool->Shutdown();
            }
            m_deviceTransientAttachmentPools.clear();
            MultiDeviceObject::Shutdown();
        }
    }

    void MultiDeviceTransientAttachmentPool::Begin(
        const TransientAttachmentPoolCompileFlags compileFlags, const TransientAttachmentStatistics::MemoryUsage* memoryHint)
    {
        m_compileFlags = compileFlags;

        m_currentScope = nullptr;
        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
        {
            deviceTransientAttachmentPool->Begin(compileFlags, memoryHint);
        }
    }

    void MultiDeviceTransientAttachmentPool::BeginScope(Scope& scopeBase)
    {
        m_currentScope = static_cast<Scope*>(&scopeBase);

        // TODO: Only call for the correct device as given by the scopeBase
        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
        {
            deviceTransientAttachmentPool->BeginScope(scopeBase);
        }

        TransientAttachmentStatistics::Scope scope;
        scope.m_scopeId = scopeBase.GetId();
        scope.m_hardwareQueueClass = scopeBase.GetHardwareQueueClass();
    }

    void MultiDeviceTransientAttachmentPool::EndScope()
    {
        m_currentScope = nullptr;
        // TODO: Only call for the correct device as given by the scopeBase
        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
        {
            deviceTransientAttachmentPool->EndScope();
        }
    }

    void MultiDeviceTransientAttachmentPool::End()
    {
        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
        {
            deviceTransientAttachmentPool->End();
        }
    }

    MultiDeviceImage* MultiDeviceTransientAttachmentPool::ActivateImage(const TransientImageDescriptor& descriptor)
    {
        MultiDeviceImage* image{};

        HashValue64 hash = descriptor.GetHash();

        // Check the cache for the image.
        if (auto attachment = m_cache.Find(static_cast<uint64_t>(hash)))
        {
            image = static_cast<MultiDeviceImage*>(attachment);
        }
        // image is not in cache. Create a new one at the placed address, and add it to the cache.
        else
        {
            // Remove any existing resource entries from the cache before adding a new one
            RemoveFromCache(descriptor.m_attachmentId);

            // Ownership is managed by the cache.
            RHI::Ptr<MultiDeviceImage> imagePtr = aznew MultiDeviceImage();
            image = imagePtr.get();

            imagePtr->Init(GetDeviceMask());

            for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
            {
                imagePtr->m_deviceObjects[deviceIndex] = deviceTransientAttachmentPool->ActivateImage(descriptor);
                if (imagePtr->GetDeviceImage(deviceIndex))
                {
                    imagePtr->SetDescriptor(imagePtr->GetDeviceImage(deviceIndex)->GetDescriptor());
                }
            }

            imagePtr->SetName(descriptor.m_attachmentId);
            m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(imagePtr));
            if (!descriptor.m_attachmentId.IsEmpty())
            {
                m_reverseLookupHash.emplace(descriptor.m_attachmentId, hash);
            }
        }

        return image;
    }

    MultiDeviceBuffer* MultiDeviceTransientAttachmentPool::ActivateBuffer(const TransientBufferDescriptor& descriptor)
    {
        MultiDeviceBuffer* buffer = nullptr;

        HashValue64 hash = descriptor.GetHash();

        // Check the cache for the buffer.
        if (auto attachment = m_cache.Find(static_cast<uint64_t>(hash)))
        {
            buffer = static_cast<MultiDeviceBuffer*>(attachment);
        }
        // buffer is not in cache. Create a new one at the placed address, and add it to the cache.
        else
        {
            // Remove any existing resource entries from the cache before adding a new one
            RemoveFromCache(descriptor.m_attachmentId);

            // Ownership is managed by the cache.
            RHI::Ptr<MultiDeviceBuffer> bufferPtr = aznew MultiDeviceBuffer();
            buffer = bufferPtr.get();

            bufferPtr->Init(GetDeviceMask());

            for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
            {
                bufferPtr->m_deviceObjects[deviceIndex] = deviceTransientAttachmentPool->ActivateBuffer(descriptor);
                if (bufferPtr->GetDeviceBuffer(deviceIndex))
                {
                    bufferPtr->SetDescriptor(bufferPtr->GetDeviceBuffer(deviceIndex)->GetDescriptor());
                }
            }

            bufferPtr->SetName(descriptor.m_attachmentId);
            m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(bufferPtr));
            if (!descriptor.m_attachmentId.IsEmpty())
            {
                m_reverseLookupHash.emplace(descriptor.m_attachmentId, hash);
            }
        }

        return buffer;
    }

    void MultiDeviceTransientAttachmentPool::DeactivateBuffer(const AttachmentId& attachmentId)
    {
        RemoveFromCache(attachmentId);

        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
        {
            deviceTransientAttachmentPool->DeactivateBuffer(attachmentId);
        }
    }

    void MultiDeviceTransientAttachmentPool::DeactivateImage(const AttachmentId& attachmentId)
    {
        RemoveFromCache(attachmentId);

        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
        {
            deviceTransientAttachmentPool->DeactivateImage(attachmentId);
        }
    }

    AZStd::unordered_map<int, TransientAttachmentStatistics> MultiDeviceTransientAttachmentPool::GetStatistics() const
    {
        AZStd::unordered_map<int, TransientAttachmentStatistics> statistics;
        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
        {
            statistics.insert({ deviceIndex, deviceTransientAttachmentPool->GetStatistics() });
        }
        return statistics;
    }

    const TransientAttachmentPoolDescriptor& MultiDeviceTransientAttachmentPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    TransientAttachmentPoolCompileFlags MultiDeviceTransientAttachmentPool::GetCompileFlags() const
    {
        return m_compileFlags;
    }

    bool MultiDeviceTransientAttachmentPool::ValidateInitParameters(
        [[maybe_unused]] const TransientAttachmentPoolDescriptor& descriptor) const
    {
#if defined(AZ_RHI_ENABLE_VALIDATION)
        switch (descriptor.m_heapParameters.m_type)
        {
        case HeapAllocationStrategy::Fixed:
            {
                if (descriptor.m_bufferBudgetInBytes == 0 && descriptor.m_imageBudgetInBytes == 0 &&
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
                    AZ_Assert(
                        false,
                        "Page size %d is bigger than budget for buffers %d",
                        pagingParameters.m_pageSizeInBytes,
                        descriptor.m_bufferBudgetInBytes);
                    return false;
                }

                if (descriptor.m_imageBudgetInBytes && pagingParameters.m_pageSizeInBytes > descriptor.m_imageBudgetInBytes)
                {
                    AZ_Assert(
                        false,
                        "Page size %d is bigger than budget for images %d",
                        pagingParameters.m_pageSizeInBytes,
                        descriptor.m_imageBudgetInBytes);
                    return false;
                }

                if (descriptor.m_renderTargetBudgetInBytes && pagingParameters.m_pageSizeInBytes > descriptor.m_renderTargetBudgetInBytes)
                {
                    AZ_Assert(
                        false,
                        "Page size %d is bigger than budget for rendertargets %d",
                        pagingParameters.m_pageSizeInBytes,
                        descriptor.m_renderTargetBudgetInBytes);
                    return false;
                }

                if (pagingParameters.m_initialAllocationPercentage && descriptor.m_bufferBudgetInBytes == 0 &&
                    descriptor.m_imageBudgetInBytes == 0 && descriptor.m_renderTargetBudgetInBytes == 0)
                {
                    AZ_Assert(
                        false,
                        "Invalid initial allocation percentage (%lf) when using a Paging allocation strategy",
                        pagingParameters.m_initialAllocationPercentage);
                    return false;
                }
                break;
            }
        case HeapAllocationStrategy::MemoryHint:
            {
                const auto& memoryHintParameters = descriptor.m_heapParameters.m_usageHintParameters;
                if (memoryHintParameters.m_heapSizeScaleFactor < 1.0f)
                {
                    AZ_Assert(
                        false,
                        "Invalid heap size scale factor (%lf) when using a MemoryHint allocation strategy",
                        memoryHintParameters.m_heapSizeScaleFactor);
                    return false;
                }

                if (memoryHintParameters.m_maxHeapWastedPercentage < 0.f || memoryHintParameters.m_maxHeapWastedPercentage > 1.f)
                {
                    AZ_Assert(
                        false,
                        "Invalid max heap wasted percentage (%lf) when using a MemoryHint allocation strategy",
                        memoryHintParameters.m_maxHeapWastedPercentage);
                    return false;
                }

                if ((memoryHintParameters.m_heapSizeScaleFactor - 1.0f) >= memoryHintParameters.m_maxHeapWastedPercentage)
                {
                    AZ_Assert(
                        false,
                        "Heap scale factor (%lf) is bugger than max wasted percentage (%lf) when using a MemoryHint allocation "
                        "strategy",
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

    void MultiDeviceTransientAttachmentPool::RemoveFromCache(AttachmentId attachmentId)
    {
        auto it = m_reverseLookupHash.find(attachmentId);
        if (it != m_reverseLookupHash.end())
        {
            HashValue64 originalHash = it->second;
            m_cache.EraseItem(aznumeric_cast<uint64_t>(originalHash));
            m_reverseLookupHash.erase(it);
        }
    }
} // namespace AZ::RHI
