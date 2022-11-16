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

        void ResourceView::release() const
        {
            // It is possible that some other thread, not us, will delete this ResourceView after we
            // decrement m_useCount. For example, another thread could create and release an instance
            // immediately after we decrement. So we copy the necessary data to the callstack before
            // decrementing. This ensures the call to ReleaseInstance() below won't crash even if this
            // ResourceView gets deleted by another thread first.
            ConstPtr<Resource> resource = m_resource;
            const int useCount = --m_useCount;

            AZ_Assert(useCount >= 0, "m_useCount is negative");

            if (useCount == 0)
            {
                if (IsInitialized())
                {
                    ResourceView* resourceView = const_cast<ResourceView*>(this);
                    // If the resource view was created independently of the cache, or if it was found in the cache,
                    // it needs to be shutdown and deleted.
                    // If EraseResourceView fails, another thread got there first, so we skip the shutdown and delete
                    if (!m_isCachedView || resource->EraseResourceView(resourceView) == ResultCode::Success)
                    {
                        resourceView->ResourceInvalidateBus::Handler::BusDisconnect(resource.get());
                        resourceView->ShutdownInternal();

                        resourceView->m_resource = nullptr;
                        resourceView->DeviceObject::Shutdown();
                        delete this;
                    }
                }
                else
                {
                    // If it's not initialized, it doesn't need to be shutdown, but it should still be deleted
                    delete this;
                }
            }
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
