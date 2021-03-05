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

        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            return RHI::PhysicalDeviceList{ aznew PhysicalDevice };
        }

    }
}
