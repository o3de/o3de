/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <Atom/RHI/ResourceView.h>
 #include <Atom/RHI/Resource.h>
 #include <Atom/RHI/DeviceImageView.h>
 #include <Atom/RHI/DeviceBufferView.h>

 namespace AZ::RHI
{
    void ResourceView::Shutdown()
        {
            if(m_resource->IsInitialized())
            {
                m_resource->EraseResourceView(static_cast<ResourceView*>(this));
                m_resource = nullptr;
            }
        }

        const Resource* ResourceView::GetResource() const
        {
            return m_resource.get();
        }

        template<typename View, typename ViewDescriptor>
        const RHI::Ptr<View> ResourceView::GetDeviceResourceView(int deviceIndex, const ViewDescriptor& viewDescriptor) const
        {
            // As this can be called concurrently and the cache is potentially manipulated, need to lock this scope
            AZStd::lock_guard lock(m_resourceViewMutex);

            // As the view keeps the device resources alive (by keeping the views alive in the cache),
            // we need to check if they are still required by the resource, otherwise delete the corresponding views from the cache
            if (m_resource->GetDeviceMask() != m_deviceMask)
            {
                m_deviceMask = m_resource->GetDeviceMask();

                MultiDeviceObject::IterateDevices(
                    m_deviceMask,
                    [this](int deviceIndex)
                    {
                        if (auto it{ m_cache.find(deviceIndex) }; it != m_cache.end())
                        {
                            m_cache.erase(it);
                        }
                        return true;
                    });
            }

            auto iterator{ m_cache.find(deviceIndex) };
            if (iterator == m_cache.end())
            {
                // ResourceView is not yet in the cache
                const auto& deviceResourceView{ m_resource->GetDeviceResource(deviceIndex)->GetResourceView(viewDescriptor) };
                auto [new_iterator, inserted]{ m_cache.insert(
                    AZStd::make_pair(deviceIndex, static_cast<DeviceResourceView*>(deviceResourceView.get()))) };
                if (inserted)
                {
                    return deviceResourceView;
                }
            }
            else if (&iterator->second->GetResource() != m_resource->GetDeviceResource(deviceIndex).get())
            {
                iterator->second =
                    static_cast<DeviceResourceView*>(m_resource->GetDeviceResource(deviceIndex)->GetResourceView(viewDescriptor).get());
            }

            return static_cast<View*>(iterator->second.get());
        }

        // Explicit instantiations for DeviceImageView and DeviceBufferView
        template const RHI::Ptr<RHI::DeviceImageView> ResourceView::GetDeviceResourceView<RHI::DeviceImageView, RHI::ImageViewDescriptor>(
            int, const RHI::ImageViewDescriptor&) const;
        template const RHI::Ptr<RHI::DeviceBufferView> ResourceView::
            GetDeviceResourceView<RHI::DeviceBufferView, RHI::BufferViewDescriptor>(int, const RHI::BufferViewDescriptor&) const;
}
