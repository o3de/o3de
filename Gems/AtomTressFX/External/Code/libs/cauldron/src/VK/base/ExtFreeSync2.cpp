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

#include "ExtFreeSync2.h"
#include "Misc/Misc.h"
#include "Misc/Error.h"

namespace CAULDRON_VK
{
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR g_vkGetPhysicalDeviceSurfaceCapabilities2KHR = NULL;
    PFN_vkGetDeviceProcAddr                        g_vkGetDeviceProcAddr = NULL;
    PFN_vkSetHdrMetadataEXT                        g_vkSetHdrMetadataEXT = NULL;
    PFN_vkAcquireFullScreenExclusiveModeEXT        g_vkAcquireFullScreenExclusiveModeEXT = NULL;
    PFN_vkReleaseFullScreenExclusiveModeEXT        g_vkReleaseFullScreenExclusiveModeEXT = NULL;
    PFN_vkGetPhysicalDeviceSurfaceFormats2KHR      g_vkGetPhysicalDeviceSurfaceFormats2KHR = NULL;
    PFN_vkSetLocalDimmingAMD                       g_vkSetLocalDimmingAMD = NULL;

    static VkSurfaceFullScreenExclusiveWin32InfoEXT s_SurfaceFullScreenExclusiveWin32InfoEXT;
    static VkSurfaceFullScreenExclusiveInfoEXT s_SurfaceFullScreenExclusiveInfoEXT;
    static VkPhysicalDeviceSurfaceInfo2KHR s_PhysicalDeviceSurfaceInfo2KHR;
    static VkDisplayNativeHdrSurfaceCapabilitiesAMD s_DisplayNativeHdrSurfaceCapabilitiesAMD;

    static bool s_isFS2DeviceExtensionsPresent = false;
    static bool s_isFS2InstanceExtensionsPresent = false;

    void ExtFreeSync2CheckInstanceExtensions(InstanceProperties *pIP)
    {
        std::vector<const char *> required_extension_names = { VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME };

        s_isFS2InstanceExtensionsPresent = true;

        for (auto& ext : required_extension_names)
        {
            if (pIP->AddInstanceExtensionName(ext) == false)
            {
                Trace(format("FreeSync2 disabled, missing instance extension: %s\n", ext));
                s_isFS2InstanceExtensionsPresent = false;
            }
        }
    }

    void ExtFreeSync2CheckDeviceExtensions(DeviceProperties *pDP)
    {
        std::vector<const char *> required_extension_names = { VK_EXT_HDR_METADATA_EXTENSION_NAME, VK_AMD_DISPLAY_NATIVE_HDR_EXTENSION_NAME, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME };

        s_isFS2DeviceExtensionsPresent = true;

        for (auto& ext : required_extension_names)
        {
            if (pDP->Add(ext) == false)
            {
                Trace(format("FreeSync2 disabled, missing device extension: %s\n", ext));
                s_isFS2DeviceExtensionsPresent = false;
            }
        }
    }

    void ExtFreeSync2GetProcAddresses(VkInstance instance, VkDevice device)
    {
        if (ExtFreeSync2AreAllExtensionsPresent())
        {
            // instance extensions
            //
            g_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
            assert(g_vkGetDeviceProcAddr);

            g_vkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
            assert(g_vkGetPhysicalDeviceSurfaceCapabilities2KHR);

            g_vkGetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormats2KHR");
            assert(g_vkGetPhysicalDeviceSurfaceFormats2KHR);

            // device extensions
            //
            g_vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)g_vkGetDeviceProcAddr(device, "vkSetHdrMetadataEXT");
            assert(g_vkSetHdrMetadataEXT);

            g_vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)g_vkGetDeviceProcAddr(device, "vkAcquireFullScreenExclusiveModeEXT");
            assert(g_vkAcquireFullScreenExclusiveModeEXT);

            g_vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)g_vkGetDeviceProcAddr(device, "vkReleaseFullScreenExclusiveModeEXT");
            assert(g_vkReleaseFullScreenExclusiveModeEXT);

            g_vkSetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)g_vkGetDeviceProcAddr(device, "vkSetLocalDimmingAMD");
            assert(g_vkSetLocalDimmingAMD);
        }
    }

    bool ExtFreeSync2AreAllExtensionsPresent()
    {
        return s_isFS2DeviceExtensionsPresent & s_isFS2InstanceExtensionsPresent;
    }
}
