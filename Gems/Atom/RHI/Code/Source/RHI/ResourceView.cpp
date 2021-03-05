/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RHI/ResourceView.h>
#include <Atom/RHI/Resource.h>
#include <AzCore/Debug/EventTrace.h>

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
            AZ_TRACE_METHOD();
            ResultCode resultCode = InvalidateInternal();
            if (resultCode == ResultCode::Success)
            {
                m_version = m_resource->GetVersion();
            }
            return resultCode;
        }
    }
}
