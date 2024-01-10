/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/SingleDeviceResource.h>
#include <Atom/RHI/SingleDeviceImage.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/SingleDeviceBuffer.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/SingleDeviceResourcePool.h>
#include <Atom/RHI/ResourceInvalidateBus.h>
#include <Atom/RHI/ResourceView.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/containers/unordered_map.h>


namespace AZ::RHI
{
    SingleDeviceResource::~SingleDeviceResource()
    {
        AZ_Assert(
            GetPool() == nullptr,
            "SingleDeviceResource '%s' is still registered on pool. %s",
            GetName().GetCStr(), GetPool()->GetName().GetCStr());
    }

    bool SingleDeviceResource::IsAttachment() const
    {
        return m_frameAttachment != nullptr;
    }

    void SingleDeviceResource::InvalidateViews()
    {
        if (!m_isInvalidationQueued)
        {
            m_isInvalidationQueued = true;
            ResourceInvalidateBus::QueueEvent(this, &ResourceInvalidateBus::Events::OnResourceInvalidate);
                
            // The resource could be destroyed before the QueueFunction runs, so do a refcount increment/decrement for safety
            add_ref();
            ResourceInvalidateBus::QueueFunction([this]()
                {
                    m_isInvalidationQueued = false;
                    release();
                });
            m_version++;
        }
    }

    uint32_t SingleDeviceResource::GetVersion() const
    {
        return m_version;
    }

    bool SingleDeviceResource::IsFirstVersion() const
    {
        return m_version == 0;
    }

    void SingleDeviceResource::SetPool(SingleDeviceResourcePool* bufferPool)
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

    const SingleDeviceResourcePool* SingleDeviceResource::GetPool() const
    {
        return m_pool;
    }

    SingleDeviceResourcePool* SingleDeviceResource::GetPool()
    {
        return m_pool;
    }

    void SingleDeviceResource::SetFrameAttachment(FrameAttachment* frameAttachment)
    {
        if (Validation::IsEnabled())
        {
            // The frame attachment has tight control over lifecycle here.
            [[maybe_unused]] const bool isAttach = (!m_frameAttachment && frameAttachment);
            [[maybe_unused]] const bool isDetach = (m_frameAttachment && !frameAttachment);
            AZ_Assert(isAttach || isDetach, "The frame attachment for resource '%s' was not assigned properly.", GetName().GetCStr());
        }

        m_frameAttachment = frameAttachment;
    }

    const FrameAttachment* SingleDeviceResource::GetFrameAttachment() const
    {
        return m_frameAttachment;
    }
        
    void SingleDeviceResource::Shutdown()
    {
        // Shutdown is delegated to the parent pool if this resource is registered on one.
        if (m_pool)
        {
            AZ_Error(
                "SingleDeviceResource",
                m_frameAttachment == nullptr,
                "The resource is currently attached on a frame graph. It is not valid "
                "to shutdown a resource while it is being used as an Attachment. The "
                "behavior is undefined.");

            m_pool->ShutdownResource(this);
        }
        DeviceObject::Shutdown();
    }
    
    Ptr<ImageView> SingleDeviceResource::GetResourceView(const ImageViewDescriptor& imageViewDescriptor) const
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

    Ptr<BufferView> SingleDeviceResource::GetResourceView(const BufferViewDescriptor& bufferViewDescriptor) const
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
            // so before we create a new intrusive_ptr, we need to use fetch_add to increment the refcount as we check the value to prevent a race
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

    Ptr<ImageView> SingleDeviceResource::InsertNewImageView(HashValue64 hash, const ImageViewDescriptor& imageViewDescriptor) const
    {
        Ptr<ImageView> imageViewPtr = RHI::Factory::Get().CreateImageView();
        RHI::ResultCode resultCode = imageViewPtr->Init(static_cast<const SingleDeviceImage&>(*this), imageViewDescriptor);
        if (resultCode == RHI::ResultCode::Success)
        {
            m_resourceViewCache[static_cast<uint64_t>(hash)] = static_cast<ResourceView*>(imageViewPtr.get());
            return imageViewPtr;
        }
        else
        {
            return nullptr;
        }
    }

    Ptr<BufferView> SingleDeviceResource::InsertNewBufferView(HashValue64 hash, const BufferViewDescriptor& bufferViewDescriptor) const
    {
        Ptr<BufferView> bufferViewPtr = RHI::Factory::Get().CreateBufferView();
        RHI::ResultCode resultCode = bufferViewPtr->Init(static_cast<const SingleDeviceBuffer&>(*this), bufferViewDescriptor);
        if (resultCode == RHI::ResultCode::Success)
        {
            m_resourceViewCache[static_cast<uint64_t>(hash)] = static_cast<ResourceView*>(bufferViewPtr.get());
            return bufferViewPtr;
        }
        else
        {
            return nullptr;
        }
    }

    void SingleDeviceResource::EraseResourceView(ResourceView* resourceView) const
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
    
    bool SingleDeviceResource::IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor)
    {
        const HashValue64 hash = imageViewDescriptor.GetHash();
        auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
        return it != m_resourceViewCache.end();
    }
    
    bool SingleDeviceResource::IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor)
    {
        const HashValue64 hash = bufferViewDescriptor.GetHash();
        auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
        return it != m_resourceViewCache.end();
    }
}
