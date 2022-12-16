/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/TransientAttachmentPool.h>

#include <Atom/RHI/AliasedAttachmentAllocator.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        bool TransientAttachmentPool::NeedsTransientAttachmentPool(const TransientAttachmentPoolDescriptor& descriptor)
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

        ResultCode TransientAttachmentPool::Init(DeviceMask deviceMask, const TransientAttachmentPoolDescriptor& descriptor)
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Error("TransientAttachmentPool", false, "TransientAttachmentPool is already initialized!");
                    return ResultCode::InvalidOperation;
                }
            }

            if (!ValidateInitParameters(descriptor))
            {
                return ResultCode::InvalidArgument;
            }

            m_descriptor = descriptor;

            return ResourcePool::Init(
                deviceMask,
                descriptor,
                [this, &descriptor]()
                {
                    ResultCode resultCode = ResultCode::Success;

                    IterateDevices(
                        [this, &descriptor, &resultCode](int deviceIndex)
                        {
                            auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                            m_deviceTransientAttachmentPools[deviceIndex] = Factory::Get().CreateTransientAttachmentPool();

                            DeviceTransientAttachmentPoolDescriptor deviceDescriptor;
                            deviceDescriptor.m_bufferBudgetInBytes = descriptor.m_bufferBudgetInBytes;
                            deviceDescriptor.m_heapParameters = descriptor.m_heapParameters;
                            deviceDescriptor.m_imageBudgetInBytes = descriptor.m_imageBudgetInBytes;
                            deviceDescriptor.m_renderTargetBudgetInBytes = descriptor.m_renderTargetBudgetInBytes;

                            resultCode = m_deviceTransientAttachmentPools[deviceIndex]->Init(*device, deviceDescriptor);

                            return resultCode == ResultCode::Success;
                        });

                    return resultCode;
                });
        }

        void TransientAttachmentPool::ShutdownInternal()
        {
            if (IsInitialized())
            {
                m_deviceTransientAttachmentPools.clear();
            }
        }

        void TransientAttachmentPool::Begin(
            const TransientAttachmentPoolCompileFlags compileFlags, const TransientAttachmentStatistics::MemoryUsage* memoryHint)
        {
            m_compileFlags = compileFlags;
            m_statistics.m_heaps.clear();
            m_statistics.m_scopes.clear();
            m_statistics.m_reservedMemory = {};

            m_currentScope = nullptr;

            for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
            {
                deviceTransientAttachmentPool->Begin(compileFlags, memoryHint);
            }
        }

        void TransientAttachmentPool::BeginScope(Scope& scopeBase)
        {
            m_currentScope = static_cast<Scope*>(&scopeBase);

            for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
            {
                deviceTransientAttachmentPool->BeginScope(scopeBase);
            }

            TransientAttachmentStatistics::Scope scope;
            scope.m_scopeId = scopeBase.GetId();
            scope.m_hardwareQueueClass = scopeBase.GetHardwareQueueClass();
            m_statistics.m_scopes.push_back(scope);
        }

        Image* TransientAttachmentPool::ActivateImage(const TransientImageDescriptor& descriptor)
        {
            Image* image = nullptr;

            HashValue64 hash = descriptor.GetHash();

            // Check the cache for the image.
            if (RHI::Resource* attachment = m_cache.Find(static_cast<uint64_t>(hash)))
            {
                image = static_cast<Image*>(attachment);
            }
            // image is not in cache. Create a new one at the placed address, and add it to the cache.
            else
            {
                // Remove any existing resource entries from the cache before adding a new one
                RemoveFromCache(descriptor.m_attachmentId);

                // Ownership is managed by the cache.
                RHI::Ptr<Image> imagePtr = aznew Image();
                image = imagePtr.get();

                RHI::ResultCode resultCode = ResourcePool::InitResource(
                    image,
                    [this, image, &descriptor]
                    {
                        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
                        {
                            image->m_deviceImages[deviceIndex] = deviceTransientAttachmentPool->ActivateImage(descriptor);
                            if (image->m_deviceImages[deviceIndex])
                                image->SetDescriptor(image->m_deviceImages[deviceIndex]->GetDescriptor());
                        }

                        return ResultCode::Success;
                    });

                if (resultCode != RHI::ResultCode::Success)
                {
                    return nullptr;
                }

                image->SetName(descriptor.m_attachmentId);
                m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(imagePtr));
                if (!descriptor.m_attachmentId.IsEmpty())
                {
                    m_reverseLookupHash.emplace(descriptor.m_attachmentId, hash);
                }
            }

            return image;
        }

        Buffer* TransientAttachmentPool::ActivateBuffer(const TransientBufferDescriptor& descriptor)
        {
            Buffer* buffer = nullptr;

            HashValue64 hash = descriptor.GetHash();

            // Check the cache for the buffer.
            if (RHI::Resource* attachment = m_cache.Find(static_cast<uint64_t>(hash)))
            {
                buffer = static_cast<Buffer*>(attachment);
            }
            // buffer is not in cache. Create a new one at the placed address, and add it to the cache.
            else
            {
                // Remove any existing resource entries from the cache before adding a new one
                RemoveFromCache(descriptor.m_attachmentId);

                // Ownership is managed by the cache.
                RHI::Ptr<Buffer> bufferPtr = aznew Buffer();
                buffer = bufferPtr.get();

                RHI::ResultCode resultCode = ResourcePool::InitResource(
                    buffer,
                    [this, buffer, &descriptor]
                    {
                        for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
                        {
                            buffer->m_deviceBuffers[deviceIndex] = deviceTransientAttachmentPool->ActivateBuffer(descriptor);
                            if (buffer->m_deviceBuffers[deviceIndex])
                                buffer->SetDescriptor(buffer->m_deviceBuffers[deviceIndex]->GetDescriptor());
                        }

                        return ResultCode::Success;
                    });

                if (resultCode != RHI::ResultCode::Success)
                {
                    return nullptr;
                }

                buffer->SetName(descriptor.m_attachmentId);
                m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(bufferPtr));
                if (!descriptor.m_attachmentId.IsEmpty())
                {
                    m_reverseLookupHash.emplace(descriptor.m_attachmentId, hash);
                }
            }

            return buffer;
        }

        void TransientAttachmentPool::DeactivateBuffer(const AttachmentId& attachmentId)
        {
            RemoveFromCache(attachmentId);

            for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
            {
                deviceTransientAttachmentPool->DeactivateBuffer(attachmentId);
            }
        }

        void TransientAttachmentPool::DeactivateImage(const AttachmentId& attachmentId)
        {
            RemoveFromCache(attachmentId);

            for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
            {
                deviceTransientAttachmentPool->DeactivateImage(attachmentId);
            }
        }

        void TransientAttachmentPool::EndScope()
        {
            m_currentScope = nullptr;
        }

        void TransientAttachmentPool::End()
        {
            for (auto& [deviceIndex, deviceTransientAttachmentPool] : m_deviceTransientAttachmentPools)
            {
                deviceTransientAttachmentPool->End();
            }
        }

        const TransientAttachmentStatistics& TransientAttachmentPool::GetStatistics() const
        {
            return m_statistics;
        }

        const TransientAttachmentPoolDescriptor& TransientAttachmentPool::GetDescriptor() const
        {
            return m_descriptor;
        }

        TransientAttachmentPoolCompileFlags TransientAttachmentPool::GetCompileFlags() const
        {
            return m_compileFlags;
        }

        bool TransientAttachmentPool::ValidateInitParameters([[maybe_unused]] const TransientAttachmentPoolDescriptor& descriptor) const
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

        void TransientAttachmentPool::RemoveFromCache(AttachmentId attachmentId)
        {
            auto it = m_reverseLookupHash.find(attachmentId);
            if (it != m_reverseLookupHash.end())
            {
                HashValue64 originalHash = it->second;
                m_cache.EraseItem(aznumeric_cast<uint64_t>(originalHash));
                m_reverseLookupHash.erase(it);
            }
        }
    }
}
