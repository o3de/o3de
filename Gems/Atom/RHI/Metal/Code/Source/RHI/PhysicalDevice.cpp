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
