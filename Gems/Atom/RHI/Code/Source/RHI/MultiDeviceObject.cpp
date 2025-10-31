/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/MultiDeviceObject.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    bool MultiDeviceObject::IsInitialized() const
    {
        return AZStd::to_underlying(m_deviceMask) != 0u;
    }

    MultiDevice::DeviceMask MultiDeviceObject::GetDeviceMask() const
    {
        return m_deviceMask;
    }

    void MultiDeviceObject::Init(MultiDevice::DeviceMask deviceMask)
    {
        m_deviceMask = deviceMask;
    }

    void MultiDeviceObject::SetNameInternal(const AZStd::string_view& name)
    {
        IterateObjects<DeviceObject>(
            [&name]([[maybe_unused]] auto deviceIndex, auto deviceObject)
            {
                AZStd::string deviceName{ AZStd::string{ name } + AZStd::to_string(deviceIndex) };
                deviceObject->SetName(Name(deviceName));
            });
    }

    void MultiDeviceObject::Shutdown()
    {
        m_deviceMask = static_cast<MultiDevice::DeviceMask>(0u);
        m_deviceObjects.clear();
    }

    int MultiDeviceObject::GetDeviceCount()
    {
        return RHI::RHISystemInterface::Get()->GetDeviceCount();
    }
} // namespace AZ::RHI
