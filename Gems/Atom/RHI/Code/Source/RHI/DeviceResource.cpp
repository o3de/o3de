/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceResource.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceResourcePool.h>
#include <Atom/RHI/ResourceInvalidateBus.h>
#include <Atom/RHI/DeviceResourceView.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/containers/unordered_map.h>


namespace AZ::RHI
{
    DeviceResource::~DeviceResource()
    {
        AZ_Assert(
            GetPool() == nullptr,
            "DeviceResource '%s' is still registered on pool. %s",
            GetName().GetCStr(), GetPool()->GetName().GetCStr());
    }

    bool DeviceResource::IsAttachment() const
    {
        return m_frameAttachment != nullptr;
    }

    void DeviceResource::InvalidateViews()
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

    uint32_t DeviceResource::GetVersion() const
    {
        return m_version;
    }

    bool DeviceResource::IsFirstVersion() const
    {
        return m_version == 0;
    }

    void DeviceResource::SetPool(DeviceResourcePool* bufferPool)
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

    const DeviceResourcePool* DeviceResource::GetPool() const
    {
        return m_pool;
    }

    DeviceResourcePool* DeviceResource::GetPool()
    {
        return m_pool;
    }

    void DeviceResource::SetFrameAttachment(FrameAttachment* frameAttachment)
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

    const FrameAttachment* DeviceResource::GetFrameAttachment() const
    {
        return m_frameAttachment;
    }
        
    void DeviceResource::Shutdown()
    {
        // Shutdown is delegated to the parent pool if this resource is registered on one.
        if (m_pool)
        {
            AZ_Error(
                "DeviceResource",
                m_frameAttachment == nullptr,
                "The resource is currently attached on a frame graph. It is not valid "
                "to shutdown a resource while it is being used as an Attachment. The "
                "behavior is undefined.");

            m_pool->ShutdownResource(this);
        }
        DeviceObject::Shutdown();
    }
    
    Ptr<DeviceImageView> DeviceResource::GetResourceView(const ImageViewDescriptor& imageViewDescriptor) const
    {
        return m_resourceViewCache.GetResourceView(this, imageViewDescriptor);
    }

    Ptr<DeviceBufferView> DeviceResource::GetResourceView(const BufferViewDescriptor& bufferViewDescriptor) const
    {
        return m_resourceViewCache.GetResourceView(this, bufferViewDescriptor);
    }

    void DeviceResource::EraseResourceView(DeviceResourceView* resourceView) const
    {
        m_resourceViewCache.EraseResourceView(resourceView);
    }
    
    bool DeviceResource::IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor)
    {
        return m_resourceViewCache.IsInResourceCache(imageViewDescriptor);
    }
    
    bool DeviceResource::IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor)
    {
        return m_resourceViewCache.IsInResourceCache(bufferViewDescriptor);
    }
}
