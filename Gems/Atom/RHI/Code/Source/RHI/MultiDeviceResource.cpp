/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/SingleDeviceBufferView.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/SingleDeviceImageView.h>
#include <Atom/RHI/MultiDeviceResource.h>
#include <Atom/RHI/MultiDeviceResourcePool.h>
#include <Atom/RHI/SingleDeviceResourceView.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/hash.h>

namespace AZ::RHI
{
    MultiDeviceResource::~MultiDeviceResource()
    {
        AZ_Assert(
            GetPool() == nullptr,
            "MultiDeviceResource '%s' is still registered on pool. %s",
            GetName().GetCStr(),
            GetPool()->GetName().GetCStr());
    }

    bool MultiDeviceResource::IsAttachment() const
    {
        return m_frameAttachment != nullptr;
    }

    uint32_t MultiDeviceResource::GetVersion() const
    {
        return m_version;
    }

    bool MultiDeviceResource::IsFirstVersion() const
    {
        return m_version == 0;
    }

    void MultiDeviceResource::InvalidateViews()
    {
        IterateObjects<SingleDeviceResource>(
            []([[maybe_unused]] auto deviceIndex, auto deviceResource)
            {
                deviceResource->InvalidateViews();
            });
    }

    void MultiDeviceResource::SetPool(MultiDeviceResourcePool* bufferPool)
    {
        m_mdPool = bufferPool;

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

    const MultiDeviceResourcePool* MultiDeviceResource::GetPool() const
    {
        return m_mdPool;
    }

    MultiDeviceResourcePool* MultiDeviceResource::GetPool()
    {
        return m_mdPool;
    }

    void MultiDeviceResource::SetFrameAttachment(FrameAttachment* frameAttachment)
    {
        if (Validation::IsEnabled())
        {
            // The frame attachment has tight control over lifecycle here.
            [[maybe_unused]] const bool isAttach = (!m_frameAttachment && frameAttachment);
            [[maybe_unused]] const bool isDetach = (m_frameAttachment && !frameAttachment);
            AZ_Assert(isAttach || isDetach, "The frame attachment for resource '%s' was not assigned properly.", GetName().GetCStr());
        }

        m_frameAttachment = frameAttachment;

        IterateObjects<SingleDeviceResource>([frameAttachment]([[maybe_unused]] auto deviceIndex, auto deviceResource)
        {
            deviceResource->SetFrameAttachment(frameAttachment);
        });
    }

    const FrameAttachment* MultiDeviceResource::GetFrameAttachment() const
    {
        return m_frameAttachment;
    }

    void MultiDeviceResource::Shutdown()
    {
        // Shutdown is delegated to the parent pool if this resource is registered on one.
        if (m_mdPool)
        {
            AZ_Error(
                "MultiDeviceResource",
                m_frameAttachment == nullptr,
                "The resource is currently attached on a frame graph. It is not valid "
                "to shutdown a resource while it is being used as an Attachment. The "
                "behavior is undefined.");

            m_mdPool->ShutdownResource(this);
        }
        MultiDeviceObject::Shutdown();
    }
} // namespace AZ::RHI
