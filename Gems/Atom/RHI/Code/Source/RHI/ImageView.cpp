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
            return ResourceView::GetDeviceResourceView<DeviceImageView>(deviceIndex, m_descriptor);
        }

        
        AZStd::unordered_map<int, uint32_t> ImageView::GetBindlessReadIndex() const
        {
            AZStd::unordered_map<int, uint32_t> result;

            MultiDeviceObject::IterateDevices(
                GetResource()->GetDeviceMask(),
                [this, &result](int deviceIndex)
                {
                    result[deviceIndex] = GetDeviceImageView(deviceIndex)->GetBindlessReadIndex();
                    return true;
                });

            return result;
        }
}