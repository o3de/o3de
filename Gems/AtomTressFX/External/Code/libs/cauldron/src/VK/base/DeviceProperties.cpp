// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "DeviceProperties.h"
#include "Misc/Misc.h"

namespace CAULDRON_VK
{    
    bool DeviceProperties::IsExtensionPresent(const char *pExtName)
    {
        return std::find_if(
            m_deviceExtensionProperties.begin(),
            m_deviceExtensionProperties.end(),
            [pExtName](const VkExtensionProperties& extensionProps) -> bool {
            return strcmp(extensionProps.extensionName, pExtName) == 0;
        }) != m_deviceExtensionProperties.end();
    }

    VkResult DeviceProperties::Init(VkPhysicalDevice physicaldevice)
    {   
        m_physicaldevice = physicaldevice;

        // Enumerate device extensions
        //
        uint32_t extensionCount;
        VkResult res = vkEnumerateDeviceExtensionProperties(physicaldevice, nullptr, &extensionCount, NULL);
        m_deviceExtensionProperties.resize(extensionCount);
        res = vkEnumerateDeviceExtensionProperties(physicaldevice, nullptr, &extensionCount, m_deviceExtensionProperties.data());

        return res;
    }

    bool DeviceProperties::Add(const char *deviceExtensionName)
    {
        if (IsExtensionPresent(deviceExtensionName))
        {
            m_device_extension_names.push_back(deviceExtensionName);
            return true;
        }

        Trace("Opps!! The device extension '%s' has not been found", deviceExtensionName);

        return false;
    }

    void  DeviceProperties::GetExtensionNamesAndConfigs(std::vector<const char *> *pDevice_extension_names)
    {
        for (auto &name : m_device_extension_names)
            pDevice_extension_names->push_back(name);
    }

}