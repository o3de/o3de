/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ResourceView.h>
#include <Atom/RHI/Resource.h>
namespace AZ
{
    namespace RHI
    {
        ResultCode ResourceView::Init(const Resource& resource)
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

        ResultCode ResourceView::Shutdown()
        {
            if (IsInitialized())
            {
                if (m_resource->EraseResourceView(this) == ResultCode::Success)
                {
                    ResourceInvalidateBus::Handler::BusDisconnect(m_resource.get());
                    ShutdownInternal();

                    m_resource = nullptr;
                    DeviceObject::Shutdown();
                }
                else
                {
                    // In the time between the refcount reaching 0 on the current thread and EraseResourceView acquiring a lock on the cache,
                    // another thread may have incremented the refcount. If so, EraseResourceView will return false,
                    // and we should skip destroying the BufferView since it's still in use. We also return false
                    // to tell the ObjectDeleter that it should not delete this object
                    return ResultCode::Fail;
                }
            }
            return ResultCode::Success;
        }

        const Resource& ResourceView::GetResource() const
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
}
