/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Factory.h>
#include <Tests/Device.h>

namespace UnitTest
{
    using namespace AZ;

    PhysicalDevice::PhysicalDevice()
    {
        m_descriptor.m_description = "UnitTest Fake Device";
    }

    Device::Device()
    {
        m_descriptor.m_platformLimitsDescriptor = aznew RHI::PlatformLimitsDescriptor;
    }

    RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
    {
        RHI::PhysicalDeviceList devices;
        for(auto deviceIndex{0}; deviceIndex < DeviceCount; ++deviceIndex)
        {
            devices.emplace_back(aznew PhysicalDevice);
        }
        return devices;
    }

    RHI::Ptr<RHI::Device> MakeTestDevice()
    {
        RHI::PhysicalDeviceList physicalDevices = RHI::Factory::Get().EnumeratePhysicalDevices();
        EXPECT_EQ(physicalDevices.size(), DeviceCount);

        RHI::Ptr<RHI::Device> device = RHI::Factory::Get().CreateDevice();
        device->Init(RHI::MultiDevice::DefaultDeviceIndex, *physicalDevices[0]);

        return device;
    }
}
