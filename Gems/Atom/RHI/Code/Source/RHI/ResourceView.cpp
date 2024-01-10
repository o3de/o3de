/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ResourceView.h>
#include <Atom/RHI/SingleDeviceResource.h>
namespace AZ::RHI
{
    ResultCode ResourceView::Init(const SingleDeviceResource& resource)
    {
        RHI::Device& device = resource.GetDevice();

        m_resource = &resource;
        m_version = resource.GetVersion();
        ResultCode resultCode = InitInternal(device, resource);
        if (resultCode != ResultCode::Success)
        {
            m_resource = nullptr;
            return resultCode;
        }

        DeviceObject::Init(device);
        ResourceInvalidateBus::Handler::BusConnect(&resource);
        return ResultCode::Success;
    }

    void ResourceView::Shutdown()
    {
        if (IsInitialized())
        {
            ResourceInvalidateBus::Handler::BusDisconnect(m_resource.get());
            ShutdownInternal();

            m_resource->EraseResourceView(this);
            m_resource = nullptr;
            DeviceObject::Shutdown();
        }
    }

    const SingleDeviceResource& ResourceView::GetResource() const
    {
        return *m_resource;
    }

    bool ResourceView::IsStale() const
    {
        return m_resource && m_resource->GetVersion() != m_version;
    }

    ResultCode ResourceView::OnResourceInvalidate()
    {
        AZ_PROFILE_FUNCTION(RHI);
        ResultCode resultCode = InvalidateInternal();
        if (resultCode == ResultCode::Success)
        {
            m_version = m_resource->GetVersion();
        }
        return resultCode;
    }
}
