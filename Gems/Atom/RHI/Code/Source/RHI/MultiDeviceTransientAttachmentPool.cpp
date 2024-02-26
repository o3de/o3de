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

        if (!SingleDeviceTransientAttachmentPool::ValidateInitParameters(descriptor))
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

                m_deviceObjects[deviceIndex] = Factory::Get().CreateTransientAttachmentPool();

                TransientAttachmentPoolDescriptor deviceDescriptor;
                deviceDescriptor.m_bufferBudgetInBytes = descriptor.m_bufferBudgetInBytes;
                deviceDescriptor.m_heapParameters = descriptor.m_heapParameters;
                deviceDescriptor.m_imageBudgetInBytes = descriptor.m_imageBudgetInBytes;
                deviceDescriptor.m_renderTargetBudgetInBytes = descriptor.m_renderTargetBudgetInBytes;

                resultCode = GetDeviceTransientAttachmentPool(deviceIndex)->Init(*device, deviceDescriptor);

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific TransientAttachmentPools and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    void MultiDeviceTransientAttachmentPool::Shutdown()
    {
        if (IsInitialized())
        {
            IterateObjects<SingleDeviceTransientAttachmentPool>(
                [](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
                {
                    deviceTransientAttachmentPool->Shutdown();
                });
            m_deviceObjects.clear();
            MultiDeviceObject::Shutdown();
            m_cache.Clear();
            m_reverseLookupHash.clear();
        }
    }

    void MultiDeviceTransientAttachmentPool::Begin(
        const TransientAttachmentPoolCompileFlags compileFlags, const TransientAttachmentStatistics::MemoryUsage* memoryHint)
    {
        m_compileFlags = compileFlags;

        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [&compileFlags, &memoryHint](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
            {
                deviceTransientAttachmentPool->Begin(compileFlags, memoryHint);
            });
    }

    void MultiDeviceTransientAttachmentPool::BeginScope(Scope& scopeBase)
    {
        // TODO: Only call for the correct device as given by the scopeBase
        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [&scopeBase](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
            {
                deviceTransientAttachmentPool->BeginScope(scopeBase);
            });
    }

    void MultiDeviceTransientAttachmentPool::EndScope()
    {
        // TODO: Only call for the correct device as given by the scopeBase
        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
            {
                deviceTransientAttachmentPool->EndScope();
            });
    }

    void MultiDeviceTransientAttachmentPool::End()
    {
        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
            {
                deviceTransientAttachmentPool->End();
            });
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

            imagePtr->SetName(descriptor.m_attachmentId);
            m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(imagePtr));
            if (!descriptor.m_attachmentId.IsEmpty())
            {
                m_reverseLookupHash.emplace(descriptor.m_attachmentId, hash);
            }
        }

        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [&image, &descriptor](auto deviceIndex, auto deviceTransientAttachmentPool)
            {
                auto deviceImage{ deviceTransientAttachmentPool->ActivateImage(descriptor) };
                if (deviceImage)
                {
                    image->m_deviceObjects[deviceIndex] = deviceImage;
                    image->SetDescriptor(deviceImage->GetDescriptor());
                }
                else
                {
                    if (auto potentialDeviceImage{ image->m_deviceObjects.find(deviceIndex) };
                        potentialDeviceImage != image->m_deviceObjects.end())
                    {
                        image->m_deviceObjects.erase(potentialDeviceImage);
                    }
                }
            });

        if (image->m_deviceObjects.empty())
        {
            // Remove the invalid image from the cache
            RemoveFromCache(descriptor.m_attachmentId);
            return nullptr;
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

            bufferPtr->SetName(descriptor.m_attachmentId);
            m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(bufferPtr));
            if (!descriptor.m_attachmentId.IsEmpty())
            {
                m_reverseLookupHash.emplace(descriptor.m_attachmentId, hash);
            }
        }

        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [&buffer, &descriptor](auto deviceIndex, auto deviceTransientAttachmentPool)
            {
                auto deviceBuffer{ deviceTransientAttachmentPool->ActivateBuffer(descriptor) };
                if (deviceBuffer)
                {
                    buffer->m_deviceObjects[deviceIndex] = deviceBuffer;
                    buffer->SetDescriptor(deviceBuffer->GetDescriptor());
                }
                else
                {
                    if (auto potentialDeviceBuffer{ buffer->m_deviceObjects.find(deviceIndex) };
                        potentialDeviceBuffer != buffer->m_deviceObjects.end())
                    {
                        buffer->m_deviceObjects.erase(potentialDeviceBuffer);
                    }
                }
            });

        if (buffer->m_deviceObjects.empty())
        {
            // Remove the invalid buffer from the cache
            RemoveFromCache(descriptor.m_attachmentId);
            return nullptr;
        }

        return buffer;
    }

    void MultiDeviceTransientAttachmentPool::DeactivateBuffer(const AttachmentId& attachmentId)
    {
        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [&attachmentId](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
            {
                deviceTransientAttachmentPool->DeactivateBuffer(attachmentId);
            });
    }

    void MultiDeviceTransientAttachmentPool::DeactivateImage(const AttachmentId& attachmentId)
    {
        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [&attachmentId](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
            {
                deviceTransientAttachmentPool->DeactivateImage(attachmentId);
            });
    }

    AZStd::unordered_map<int, TransientAttachmentStatistics> MultiDeviceTransientAttachmentPool::GetStatistics() const
    {
        AZStd::unordered_map<int, TransientAttachmentStatistics> statistics;
        IterateObjects<SingleDeviceTransientAttachmentPool>(
            [&statistics](auto deviceIndex, auto deviceTransientAttachmentPool)
            {
                statistics.insert({ deviceIndex, deviceTransientAttachmentPool->GetStatistics() });
            });
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
