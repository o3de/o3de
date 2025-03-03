/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/DeviceResourceView.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/ResourcePool.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/hash.h>

namespace AZ::RHI
{
    Resource::~Resource()
    {
        AZ_Assert(
            GetPool() == nullptr,
            "Resource '%s' is still registered on pool. %s",
            GetName().GetCStr(),
            GetPool()->GetName().GetCStr());
    }

    bool Resource::IsAttachment() const
    {
        return m_frameAttachment != nullptr;
    }

    uint32_t Resource::GetVersion() const
    {
        return m_version;
    }

    bool Resource::IsFirstVersion() const
    {
        return m_version == 0;
    }

    void Resource::InvalidateViews()
    {
        IterateObjects<DeviceResource>(
            []([[maybe_unused]] auto deviceIndex, auto deviceResource)
            {
                deviceResource->InvalidateViews();
            });
    }

    void Resource::SetPool(ResourcePool* bufferPool)
    {
        m_pool = bufferPool;

        const bool isValidPool = bufferPool != nullptr;
        if (isValidPool)
        {
            // Only invalidate the resource if it has dependent views. It
            // can't have any if this is the first initialization.
            if (!IsFirstVersion())
            {
                InvalidateViews();
            }
        }

        ++m_version;
    }

    const ResourcePool* Resource::GetPool() const
    {
        return m_pool;
    }

    ResourcePool* Resource::GetPool()
    {
        return m_pool;
    }

    void Resource::SetFrameAttachment(FrameAttachment* frameAttachment, int deviceIndex)
    {
        m_frameAttachment = frameAttachment;

        if (deviceIndex >= 0)
        {
            GetDeviceObject<DeviceResource>(deviceIndex)->SetFrameAttachment(frameAttachment);
        }
        else
        {
            // If we don't get a valid device index, we must assume that the resource may be used on
            // any device, so we set it for all of them.
            IterateObjects<DeviceResource>(
                [frameAttachment]([[maybe_unused]] auto deviceIndex, auto deviceResource)
                {
                    deviceResource->SetFrameAttachment(frameAttachment);
                });
        }
    }

    const FrameAttachment* Resource::GetFrameAttachment() const
    {
        return m_frameAttachment;
    }

    void Resource::Shutdown()
    {
        // Shutdown is delegated to the parent pool if this resource is registered on one.
        if (m_pool)
        {
            AZ_Error(
                "Resource",
                m_frameAttachment == nullptr,
                "The resource is currently attached on a frame graph. It is not valid "
                "to shutdown a resource while it is being used as an Attachment. The "
                "behavior is undefined.");

            m_pool->ShutdownResource(this);
        }
        MultiDeviceObject::Shutdown();
    }

    Ptr<ImageView> Resource::GetResourceView(const ImageViewDescriptor& imageViewDescriptor) const
    {
        const HashValue64 hash = imageViewDescriptor.GetHash();
        AZStd::lock_guard<AZStd::mutex> registryLock(m_cacheMutex);
        auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
        if (it == m_resourceViewCache.end())
        {
            return InsertNewImageView(hash, imageViewDescriptor);
        }
        else
        {
            // We've found a matching ResourceView in the cache, but another thread may be releasing the last intrusive_ptr while
            // we are in this function, dropping the refcount to 0 (and forcing it to -1 for good measure) then deleting it.
            //
            //  There are 2 scenarios:
            //
            // m_useCount is -1. The other thread is already on the path to deleting it. We need to make a new one here
            // and replace the old one.
            //
            // m_useCount is >=0. We cannot guarantee another thread won't drop the refcount to 0 after we check the value here
            // so before we create a new intrusive_ptr, we need to use fetch_add to increment the refcount as we check the value to
            // prevent a race
            int useCount = it->second->m_useCount.fetch_add(2);
            if (useCount == -1)
            {
                // The useCount was -1 before we incremented.
                // Another thread is going to come along and delete the one we just found.
                // Go ahead and erase it and insert a new one instead
                m_resourceViewCache.erase(it);

                return InsertNewImageView(hash, imageViewDescriptor);
            }
            else
            {
                // Create the new Ptr, increasing the refcount
                Ptr<ImageView> result = static_cast<ImageView*>(it->second);

                // Before we checked the value we artificially incremented the refcount to prevent another
                // thread from letting it go to 0 again. Get rid of that artificial increase now that we have
                // our new Ptr to hold on to the refcount
                it->second->m_useCount.fetch_sub(2);
                return result;
            }
        }
    }

    Ptr<BufferView> Resource::GetResourceView(const BufferViewDescriptor& bufferViewDescriptor) const
    {
        const HashValue64 hash = bufferViewDescriptor.GetHash();
        AZStd::lock_guard<AZStd::mutex> registryLock(m_cacheMutex);
        auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
        if (it == m_resourceViewCache.end())
        {
            return InsertNewBufferView(hash, bufferViewDescriptor);
        }
        else
        {
            // We've found a matching ResourceView in the cache, but another thread may be releasing the last intrusive_ptr while
            // we are in this function, dropping the refcount to 0 (and forcing it to -1 for good measure) then deleting it.
            //
            //  There are 2 scenarios:
            //
            // m_useCount is -1. The other thread is already on the path to deleting it. We need to make a new one here
            // and replace the old one.
            //
            // m_useCount is >=0. We cannot guarantee another thread won't drop the refcount to 0 after we check the value here
            // so before we create a new intrusive_ptr, we need to use fetch_add to increment the refcount as we check the value to prevent
            // a race
            int useCount = it->second->m_useCount.fetch_add(2);
            if (useCount == -1)
            {
                // The useCount was -1 before we incremented.
                // Another thread is going to come along and delete the one we just found.
                // Go ahead and erase it and insert a new one instead
                m_resourceViewCache.erase(it);

                return InsertNewBufferView(hash, bufferViewDescriptor);
            }
            else
            {
                // Create the new Ptr, increasing the refcount
                Ptr<BufferView> result = static_cast<BufferView*>(it->second);

                // Before we checked the value we artificially incremented the refcount to prevent another
                // thread from letting it go to 0 again. Get rid of that artificial increase now that we have
                // our new Ptr to hold on to the refcount
                it->second->m_useCount.fetch_sub(2);
                return result;
            }
        }
    }

    Ptr<ImageView> Resource::InsertNewImageView(HashValue64 hash, const ImageViewDescriptor& imageViewDescriptor) const
    {
        Ptr<ImageView> imageViewPtr = aznew ImageView{ static_cast<const RHI::Image*>(this), imageViewDescriptor, GetDeviceMask() };
        if (imageViewPtr)
        {
            m_resourceViewCache[static_cast<uint64_t>(hash)] = static_cast<ResourceView*>(imageViewPtr.get());
            return imageViewPtr;
        }
        else
        {
            return nullptr;
        }
    }

    Ptr<BufferView> Resource::InsertNewBufferView(HashValue64 hash, const BufferViewDescriptor& bufferViewDescriptor) const
    {
        Ptr<BufferView> bufferViewPtr = aznew BufferView{static_cast<const RHI::Buffer*>(this), bufferViewDescriptor, GetDeviceMask()};
        if (bufferViewPtr)
        {
            m_resourceViewCache[static_cast<uint64_t>(hash)] = static_cast<ResourceView*>(bufferViewPtr.get());
            return bufferViewPtr;
        }
        else
        {
            return nullptr;
        }
    }

    void Resource::EraseResourceView(ResourceView* resourceView) const
    {
        AZStd::lock_guard<AZStd::mutex> registryLock(m_cacheMutex);
        auto itr = m_resourceViewCache.begin();
        while (itr != m_resourceViewCache.end())
        {
            if (itr->second == resourceView)
            {
                m_resourceViewCache.erase(itr->first);
                break;
            }
            itr++;
        }
    }

    bool Resource::IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor)
    {
        const HashValue64 hash = imageViewDescriptor.GetHash();
        auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
        return it != m_resourceViewCache.end();
    }

    bool Resource::IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor)
    {
        const HashValue64 hash = bufferViewDescriptor.GetHash();
        auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
        return it != m_resourceViewCache.end();
    }
} // namespace AZ::RHI
