/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
    {
        return RHI::PhysicalDeviceList{aznew PhysicalDevice};
    }

    RHI::Ptr<RHI::Device> MakeTestDevice()
    {
        RHI::PhysicalDeviceList physicalDevices = RHI::Factory::Get().EnumeratePhysicalDevices();
        EXPECT_EQ(physicalDevices.size(), 1);

        RHI::Ptr<RHI::Device> device = RHI::Factory::Get().CreateDevice();
        device->Init(*physicalDevices[0]);
        device->PostInit(RHI::DeviceDescriptor{});

        return device;
    }
}
