/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 #include <Atom/RHI/BufferView.h>
 #include <Atom/RHI/Buffer.h>

 namespace AZ::RHI
 {
    //! Given a device index, return the corresponding DeviceBufferView for the selected device
    const RHI::Ptr<RHI::DeviceBufferView> BufferView::GetDeviceBufferView(int deviceIndex) const
    {
        return ResourceView::GetDeviceResourceView<DeviceBufferView>(deviceIndex, m_descriptor);
    }

    AZStd::unordered_map<int, uint32_t> BufferView::GetBindlessReadIndex() const
    {
        AZStd::unordered_map<int, uint32_t> result;

        MultiDeviceObject::IterateDevices(
            GetResource()->GetDeviceMask(),
            [this, &result](int deviceIndex)
            {
                result[deviceIndex] = GetDeviceBufferView(deviceIndex)->GetBindlessReadIndex();
                return true;
            });

        return result;
    }
 }