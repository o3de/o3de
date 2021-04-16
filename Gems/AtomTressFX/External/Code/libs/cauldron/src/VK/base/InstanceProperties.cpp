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
#include "InstanceProperties.h"
#include "Misc/Misc.h"

namespace CAULDRON_VK
{    
    bool InstanceProperties::IsLayerPresent(const char *pExtName)
    {
        return std::find_if(
            m_instanceLayerProperties.begin(),
            m_instanceLayerProperties.end(),
            [pExtName](const VkLayerProperties& layerProps) -> bool {
            return strcmp(layerProps.layerName, pExtName) == 0;
        }) != m_instanceLayerProperties.end();
    }

    bool InstanceProperties::IsExtensionPresent(const char *pExtName)
    {
        return std::find_if(
            m_instanceExtensionProperties.begin(),
            m_instanceExtensionProperties.end(),
            [pExtName](const VkExtensionProperties& extensionProps) -> bool {
            return strcmp(extensionProps.extensionName, pExtName) == 0;
        }) != m_instanceExtensionProperties.end();
    }

    VkResult InstanceProperties::Init()
    {   
        // Query instance layers.
        //
        uint32_t instanceLayerPropertyCount = 0;
        VkResult res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, nullptr);
        m_instanceLayerProperties.resize(instanceLayerPropertyCount);
        assert(res == VK_SUCCESS);        
        if (instanceLayerPropertyCount > 0)
        {
            res = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, m_instanceLayerProperties.data());
            assert(res == VK_SUCCESS);
        }

        // Query instance extensions.
        //
        uint32_t instanceExtensionPropertyCount = 0;
        res = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, nullptr);
        assert(res == VK_SUCCESS);
        m_instanceExtensionProperties.resize(instanceExtensionPropertyCount);
        if (instanceExtensionPropertyCount > 0)
        {
            res = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, m_instanceExtensionProperties.data());
            assert(res == VK_SUCCESS);
        }

        return res;
    }

    bool InstanceProperties::AddInstanceLayerName(const char *instanceLayerName)
    {
        if (IsLayerPresent(instanceLayerName))
        {
            m_instance_layer_names.push_back(instanceLayerName);
            return true;
        }

        Trace("Opps!! The instance layer '%s' has not been found\n", instanceLayerName);

        return false;
    }

    bool InstanceProperties::AddInstanceExtensionName(const char *instanceExtensionName)
    {
        if (IsExtensionPresent(instanceExtensionName))
        {
            m_instance_extension_names.push_back(instanceExtensionName);
            return true;
        }

        Trace("Opps!! The instance extension '%s' has not been found\n", instanceExtensionName);

        return false;
    }

    void  InstanceProperties::GetExtensionNamesAndConfigs(std::vector<const char *> *pInstance_layer_names, std::vector<const char *> *pInstance_extension_names)
    {
        for (auto &name : m_instance_layer_names)
            pInstance_layer_names->push_back(name);

        for (auto &name : m_instance_extension_names)
            pInstance_extension_names->push_back(name);
    }

}