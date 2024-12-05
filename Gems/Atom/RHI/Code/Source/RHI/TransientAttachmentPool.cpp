/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/AliasedAttachmentAllocator.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    ResultCode TransientAttachmentPool::Init(
        MultiDevice::DeviceMask deviceMask, const AZStd::unordered_map<int, TransientAttachmentPoolDescriptor>& descriptors)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("TransientAttachmentPool", false, "TransientAttachmentPool is already initialized!");
                return ResultCode::InvalidOperation;
            }
        }

        m_descriptors = descriptors;

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode = ResultCode::Success;

        IterateDevices(
            [this, &resultCode](int deviceIndex)
            {
                if (!DeviceTransientAttachmentPool::ValidateInitParameters(m_descriptors[deviceIndex]))
                {
                    return false;
                }

                auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                m_deviceObjects[deviceIndex] = Factory::Get().CreateTransientAttachmentPool();

                resultCode = GetDeviceTransientAttachmentPool(deviceIndex)->Init(*device, m_descriptors[deviceIndex]);

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific TransientAttachmentPools and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }

        return resultCode;
    }

    void TransientAttachmentPool::Shutdown()
    {
        if (IsInitialized())
        {
            MultiDeviceObject::Shutdown();
            m_cache.Clear();
            m_reverseLookupHash.clear();
        }
    }

    void TransientAttachmentPool::Begin(
        const TransientAttachmentPoolCompileFlags compileFlags, const TransientAttachmentStatistics::MemoryUsage* memoryHint)
    {
        m_compileFlags = compileFlags;

        IterateObjects<DeviceTransientAttachmentPool>(
            [&compileFlags, &memoryHint](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
            {
                deviceTransientAttachmentPool->Begin(compileFlags, memoryHint);
            });
    }

    void TransientAttachmentPool::BeginScope(Scope& scopeBase)
    {
        m_currentScope = &scopeBase;

        GetDeviceTransientAttachmentPool(m_currentScope->GetDeviceIndex())->BeginScope(scopeBase);
    }

    void TransientAttachmentPool::EndScope()
    {
        GetDeviceTransientAttachmentPool(m_currentScope->GetDeviceIndex())->EndScope();

        m_currentScope = nullptr;
    }

    void TransientAttachmentPool::End()
    {
        IterateObjects<DeviceTransientAttachmentPool>(
            [](auto /*deviceIndex*/, auto deviceTransientAttachmentPool)
            {
                deviceTransientAttachmentPool->End();
            });
    }

    Image* TransientAttachmentPool::ActivateImage(const TransientImageDescriptor& descriptor)
    {
        Image* image{};

        HashValue64 hash = descriptor.GetHash();

        // Check the cache for the image.
        if (auto attachment = m_cache.Find(static_cast<uint64_t>(hash)))
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

            imagePtr->Init(MultiDevice::DeviceMask{ 0 });

            imagePtr->SetName(descriptor.m_attachmentId);
            m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(imagePtr));
            if (!descriptor.m_attachmentId.IsEmpty())
            {
                m_reverseLookupHash.emplace(descriptor.m_attachmentId, hash);
            }
        }

        auto deviceIndex = m_currentScope->GetDeviceIndex();
        auto deviceTransientAttachmentPool = GetDeviceTransientAttachmentPool(deviceIndex);

        auto deviceImage{ deviceTransientAttachmentPool->ActivateImage(descriptor) };
        if (deviceImage)
        {
            image->m_deviceObjects[deviceIndex] = deviceImage;
            image->SetDescriptor(deviceImage->GetDescriptor());
            image->Init(image->GetDeviceMask() | MultiDevice::DeviceMask(1 << deviceIndex));

            if (const auto& name = image->GetName(); !name.IsEmpty())
            {
                image->m_deviceObjects[deviceIndex]->SetName(name);
            }
        }
        else if (!CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            if (auto potentialDeviceImage{ image->m_deviceObjects.find(deviceIndex) }; potentialDeviceImage != image->m_deviceObjects.end())
            {
                image->m_deviceObjects.erase(potentialDeviceImage);
            }
        }

        if (image->m_deviceObjects.empty())
        {
            // Remove the invalid image from the cache
            RemoveFromCache(descriptor.m_attachmentId);
            return nullptr;
        }

        return image;
    }

    Buffer* TransientAttachmentPool::ActivateBuffer(const TransientBufferDescriptor& descriptor)
    {
        Buffer* buffer = nullptr;

        HashValue64 hash = descriptor.GetHash();

        // Check the cache for the buffer.
        if (auto attachment = m_cache.Find(static_cast<uint64_t>(hash)))
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

            bufferPtr->Init(MultiDevice::DeviceMask{ 0 });

            bufferPtr->SetName(descriptor.m_attachmentId);
            m_cache.Insert(static_cast<uint64_t>(hash), AZStd::move(bufferPtr));
            if (!descriptor.m_attachmentId.IsEmpty())
            {
                m_reverseLookupHash.emplace(descriptor.m_attachmentId, hash);
            }
        }

        auto deviceIndex = m_currentScope->GetDeviceIndex();
        auto deviceTransientAttachmentPool = GetDeviceTransientAttachmentPool(deviceIndex);

        auto deviceBuffer{ deviceTransientAttachmentPool->ActivateBuffer(descriptor) };
        if (deviceBuffer)
        {
            buffer->m_deviceObjects[deviceIndex] = deviceBuffer;
            buffer->SetDescriptor(deviceBuffer->GetDescriptor());
            buffer->Init(buffer->GetDeviceMask() | MultiDevice::DeviceMask(1 << deviceIndex));

            if (const auto& name = buffer->GetName(); !name.IsEmpty())
            {
                buffer->m_deviceObjects[deviceIndex]->SetName(name);
            }
        }
        else if (!CheckBitsAny(m_compileFlags, TransientAttachmentPoolCompileFlags::DontAllocateResources))
        {
            if (auto potentialDeviceBuffer{ buffer->m_deviceObjects.find(deviceIndex) };
                potentialDeviceBuffer != buffer->m_deviceObjects.end())
            {
                buffer->m_deviceObjects.erase(potentialDeviceBuffer);
            }
        }

        if (buffer->m_deviceObjects.empty())
        {
            // Remove the invalid buffer from the cache
            RemoveFromCache(descriptor.m_attachmentId);
            return nullptr;
        }

        return buffer;
    }

    void TransientAttachmentPool::DeactivateBuffer(const AttachmentId& attachmentId)
    {
        GetDeviceTransientAttachmentPool(m_currentScope->GetDeviceIndex())->DeactivateBuffer(attachmentId);
    }

    void TransientAttachmentPool::DeactivateImage(const AttachmentId& attachmentId)
    {
        GetDeviceTransientAttachmentPool(m_currentScope->GetDeviceIndex())->DeactivateImage(attachmentId);
    }

    void TransientAttachmentPool::RemoveDeviceBuffer(int deviceIndex, Buffer* buffer)
    {
        buffer->Init(ResetBit(buffer->GetDeviceMask(), deviceIndex));
        buffer->m_deviceObjects.erase(deviceIndex);
    }

    void TransientAttachmentPool::RemoveDeviceImage(int deviceIndex, Image* image)
    {
        image->Init(ResetBit(image->GetDeviceMask(), deviceIndex));
        image->m_deviceObjects.erase(deviceIndex);
    }

    AZStd::unordered_map<int, TransientAttachmentStatistics> TransientAttachmentPool::GetStatistics() const
    {
        AZStd::unordered_map<int, TransientAttachmentStatistics> statistics;
        IterateObjects<DeviceTransientAttachmentPool>(
            [&statistics](auto deviceIndex, auto deviceTransientAttachmentPool)
            {
                statistics.insert({ deviceIndex, deviceTransientAttachmentPool->GetStatistics() });
            });
        return statistics;
    }

    const AZStd::unordered_map<int, TransientAttachmentPoolDescriptor>& TransientAttachmentPool::GetDescriptor() const
    {
        return m_descriptors;
    }

    TransientAttachmentPoolCompileFlags TransientAttachmentPool::GetCompileFlags() const
    {
        return m_compileFlags;
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
} // namespace AZ::RHI
