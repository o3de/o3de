/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/ResourcePool.h>
#include <Atom/RHI/DeviceResourceView.h>
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
        m_Pool = bufferPool;

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
        return m_Pool;
    }

    ResourcePool* Resource::GetPool()
    {
        return m_Pool;
    }

    void Resource::SetFrameAttachment(FrameAttachment* frameAttachment)
    {
        if (Validation::IsEnabled())
        {
            // The frame attachment has tight control over lifecycle here.
            [[maybe_unused]] const bool isAttach = (!m_frameAttachment && frameAttachment);
            [[maybe_unused]] const bool isDetach = (m_frameAttachment && !frameAttachment);
            AZ_Assert(isAttach || isDetach, "The frame attachment for resource '%s' was not assigned properly.", GetName().GetCStr());
        }

        m_frameAttachment = frameAttachment;

        IterateObjects<DeviceResource>([frameAttachment]([[maybe_unused]] auto deviceIndex, auto deviceResource)
        {
            deviceResource->SetFrameAttachment(frameAttachment);
        });
    }

    const FrameAttachment* Resource::GetFrameAttachment() const
    {
        return m_frameAttachment;
    }

    void Resource::Shutdown()
    {
        // Shutdown is delegated to the parent pool if this resource is registered on one.
        if (m_Pool)
        {
            AZ_Error(
                "Resource",
                m_frameAttachment == nullptr,
                "The resource is currently attached on a frame graph. It is not valid "
                "to shutdown a resource while it is being used as an Attachment. The "
                "behavior is undefined.");

            m_Pool->ShutdownResource(this);
        }
        MultiDeviceObject::Shutdown();
    }
} // namespace AZ::RHI
