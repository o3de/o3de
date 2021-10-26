/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Common/RHI/Stubs.h>

namespace UnitTest
{
    namespace StubRHI
    {
        using namespace AZ;

        PhysicalDevice::PhysicalDevice()
        {
            m_descriptor.m_type = RHI::PhysicalDeviceType::Fake;
            m_descriptor.m_description = "UnitTest Fake Device";
        }

        Device::Device()
        {
            m_descriptor.m_platformLimitsDescriptor = aznew RHI::PlatformLimitsDescriptor;
        }

        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            return RHI::PhysicalDeviceList{ aznew PhysicalDevice };
        }

    }
}
