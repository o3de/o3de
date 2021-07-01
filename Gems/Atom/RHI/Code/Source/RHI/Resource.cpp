/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/ResourceInvalidateBus.h>
#include <Atom/RHI/ResourceView.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/containers/unordered_map.h>


namespace AZ
{
    namespace RHI
    {
        Resource::~Resource()
        {
            AZ_Assert(
                GetPool() == nullptr,
                "Resource '%s' is still registered on pool. %s",
                GetName().GetCStr(), GetPool()->GetName().GetCStr());
        }

        bool Resource::IsAttachment() const
        {
            return m_frameAttachment != nullptr;
        }

        void Resource::InvalidateViews()
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

        uint32_t Resource::GetVersion() const
        {
            return m_version;
        }

        bool Resource::IsFirstVersion() const
        {
            return m_version == 0;
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

        void Resource::SetFrameAttachment(FrameAttachment* frameAttachment)
        {
            if (Validation::IsEnabled())
            {
                // The frame attachment has tight control over lifecycle here.
                const bool isAttach = (!m_frameAttachment && frameAttachment);
                const bool isDetach = (m_frameAttachment && !frameAttachment);
                AZ_Assert(isAttach || isDetach, "The frame attachment for resource '%s' was not assigned properly.");
            }

            m_frameAttachment = frameAttachment;
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
                    "ResourceBase",
                    m_frameAttachment == nullptr,
                    "The resource is currently attached on a frame graph. It is not valid "
                    "to shutdown a resource while it is being used as an Attachment. The "
                    "behavior is undefined.");

                m_pool->ShutdownResource(this);
            }
            DeviceObject::Shutdown();
        }
    
        Ptr<ImageView> Resource::GetResourceView(const ImageViewDescriptor& imageViewDescriptor) const
        {
            const HashValue64 hash = imageViewDescriptor.GetHash();
            AZStd::lock_guard<AZStd::mutex> registryLock(m_cacheMutex);
            auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
            if (it == m_resourceViewCache.end())
            {
                Ptr<ImageView> imageViewPtr = RHI::Factory::Get().CreateImageView();
                RHI::ResultCode resultCode = imageViewPtr->Init(static_cast<const Image&>(*this), imageViewDescriptor);
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
            else
            {
                return static_cast<ImageView*>(it->second);
            }             
        }
    
        Ptr<BufferView> Resource::GetResourceView(const BufferViewDescriptor& bufferViewDescriptor) const
        {
            const HashValue64 hash = bufferViewDescriptor.GetHash();
            AZStd::lock_guard<AZStd::mutex> registryLock(m_cacheMutex);
            auto it = m_resourceViewCache.find(static_cast<uint64_t>(hash));
            if (it == m_resourceViewCache.end())
            {
                Ptr<BufferView> bufferViewPtr = RHI::Factory::Get().CreateBufferView();
                RHI::ResultCode resultCode = bufferViewPtr->Init(static_cast<const Buffer&>(*this), bufferViewDescriptor);
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
            else
            {
                return static_cast<BufferView*>(it->second);
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
    }
}
