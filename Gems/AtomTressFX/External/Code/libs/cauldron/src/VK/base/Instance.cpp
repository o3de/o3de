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
#include <algorithm>
#include "Instance.h"
#include "InstanceProperties.h"
#include <vulkan/vulkan_win32.h>
#include "ExtValidation.h"
#include "ExtFreeSync2.h"


namespace CAULDRON_VK
{
    VkInstance CreateInstance(VkApplicationInfo app_info, bool usingValidationLayer)
    {
        VkInstance instance;

        //populate list from enabled extensions
        void *pNext = NULL;

        // read layer and instance properties
        InstanceProperties ip;
        ip.Init();

        // Check required extensions are present
        //
        ip.AddInstanceExtensionName(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        ip.AddInstanceExtensionName(VK_KHR_SURFACE_EXTENSION_NAME);
        ExtFreeSync2CheckInstanceExtensions(&ip);
        if (usingValidationLayer)
        {
            usingValidationLayer = ExtDebugReportCheckInstanceExtensions(&ip, &pNext);
        }

        // prepare existing extensions and layer names into a buffer for vkCreateInstance
        std::vector<const char *> instance_layer_names;
        std::vector<const char *> instance_extension_names;
        ip.GetExtensionNamesAndConfigs(&instance_layer_names, &instance_extension_names);

        // do create the instance
        VkInstanceCreateInfo inst_info = {};
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pNext = pNext;
        inst_info.flags = 0;
        inst_info.pApplicationInfo = &app_info;
        inst_info.enabledLayerCount = (uint32_t)instance_layer_names.size();
        inst_info.ppEnabledLayerNames = (uint32_t)instance_layer_names.size() ? instance_layer_names.data() : NULL;
        inst_info.enabledExtensionCount = (uint32_t)instance_extension_names.size();
        inst_info.ppEnabledExtensionNames = instance_extension_names.data();           
        VkResult res = vkCreateInstance(&inst_info, NULL, &instance);
        assert(res == VK_SUCCESS);

        // Init the extensions (if they have been enabled successfuly)
        //
        //
        ExtDebugReportGetProcAddresses(instance);
        ExtDebugReportOnCreate(instance);

        return instance;
    }

    void DestroyInstance(VkInstance instance)
    {
        ExtDebugReportOnDestroy(instance);

        vkDestroyInstance(instance, nullptr);
    }
}



