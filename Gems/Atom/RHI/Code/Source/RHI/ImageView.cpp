/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <Atom/RHI/ImageView.h>
 #include <Atom/RHI/Image.h>

 namespace AZ::RHI
{
        //! Given a device index, return the corresponding DeviceBufferView for the selected device
        const RHI::Ptr<RHI::DeviceImageView> ImageView::GetDeviceImageView(int deviceIndex) const
        {
            AZStd::lock_guard lock(m_imageViewMutex);
    
            if (m_image->GetDeviceMask() != m_deviceMask)
            {
                m_deviceMask = m_image->GetDeviceMask();
    
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
                //! Image view is not yet in the cache
                auto [new_iterator, inserted]{ m_cache.insert(
                    AZStd::make_pair(deviceIndex, m_image->GetDeviceImage(deviceIndex)->GetImageView(m_descriptor))) };
                if (inserted)
                {
                    return new_iterator->second;
                }
            }
            else if (&iterator->second->GetImage() != m_image->GetDeviceImage(deviceIndex).get())
            {
                iterator->second = m_image->GetDeviceImage(deviceIndex)->GetImageView(m_descriptor);
            }
    
            return iterator->second;
        }

        void ImageView::Shutdown()
        {
            if(m_image->IsInitialized())
            {
                m_image->EraseResourceView(static_cast<ResourceView*>(this));
                m_image = nullptr;
            }
        }
}