/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/conversions.h>
#include <RHI/PhysicalDevice.h>

namespace Platform
{
    AZ::RHI::PhysicalDeviceType GetPhysicalDeviceType(id<MTLDevice> mtlDevice);
    AZ::RHI::PhysicalDeviceList EnumerateDevices();
}


namespace AZ
{
    namespace Metal
    {
        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            return Platform::EnumerateDevices();
        }

        void PhysicalDevice::Init(id<MTLDevice> mtlDevice)
        {
            if(mtlDevice)
            {
                NSString * deviceName = [mtlDevice name];
                const char * secondName = [ deviceName UTF8String ];
                m_descriptor.m_description = AZStd::string(secondName);
                m_descriptor.m_deviceId = [mtlDevice registryID];
                
                //Currently no way of knowing vendor id through metal. Using AMD as a placeholder for now.
                m_descriptor.m_vendorId = RHI::VendorId::AMD;
                
                m_descriptor.m_type = Platform::GetPhysicalDeviceType(mtlDevice);
            }
        }

        void PhysicalDevice::Shutdown()
        {
        }
    }
}
