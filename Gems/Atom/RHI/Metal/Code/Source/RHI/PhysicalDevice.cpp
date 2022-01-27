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
        id<MTLDevice> PhysicalDevice::GetNativeDevice()
        {
            return m_mtlNativeDevice;
        }
    
        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            return Platform::EnumerateDevices();
        }

        void PhysicalDevice::Init(id<MTLDevice> mtlDevice)
        {
            if(mtlDevice)
            {
                m_mtlNativeDevice = mtlDevice;
                NSString * deviceName = [mtlDevice name];
                const char * secondName = [ deviceName UTF8String ];
                m_descriptor.m_description = AZStd::string(secondName);
                m_descriptor.m_deviceId = [mtlDevice registryID];
 
                if(strstr(m_descriptor.m_description.c_str(), ToString(RHI::VendorId::Intel).data()))
                {
                    m_descriptor.m_vendorId = RHI::VendorId::Intel;
                }
                else if(strstr(m_descriptor.m_description.c_str(), ToString(RHI::VendorId::nVidia).data()))
                {
                    m_descriptor.m_vendorId = RHI::VendorId::nVidia;
                }
                else if(strstr(m_descriptor.m_description.c_str(), ToString(RHI::VendorId::AMD).data()))
                {
                    m_descriptor.m_vendorId = RHI::VendorId::AMD;
                }
                else if(strstr(m_descriptor.m_description.c_str(), ToString(RHI::VendorId::Qualcomm).data()))
                {
                    m_descriptor.m_vendorId = RHI::VendorId::Qualcomm;
                }
                else if(strstr(m_descriptor.m_description.c_str(), ToString(RHI::VendorId::Samsung).data()))
                {
                    m_descriptor.m_vendorId = RHI::VendorId::Samsung;
                }
                else if(strstr(m_descriptor.m_description.c_str(), ToString(RHI::VendorId::ARM).data()))
                {
                    m_descriptor.m_vendorId = RHI::VendorId::ARM;
                }
                else if(strstr(m_descriptor.m_description.c_str(), ToString(RHI::VendorId::A13).data()))
                {
                    m_descriptor.m_vendorId = RHI::VendorId::A13;
                }

                m_descriptor.m_type = Platform::GetPhysicalDeviceType(mtlDevice);
                
                NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
                AZStd::string concatVer = AZStd::string::format("%li%li%li", version.majorVersion, version.minorVersion, version.patchVersion);
                m_descriptor.m_driverVersion = AZStd::stoi(concatVer);
            }
        }

        void PhysicalDevice::Shutdown()
        {
        }
    }
}
