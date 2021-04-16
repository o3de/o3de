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
#include "ExtDebugMarkers.h"


namespace CAULDRON_VK
{
    PFN_vkDebugMarkerSetObjectTagEXT    vkDebugMarkerSetObjectTag = NULL;
    PFN_vkDebugMarkerSetObjectNameEXT   vkDebugMarkerSetObjectName = NULL;
    PFN_vkCmdDebugMarkerBeginEXT        vkCmdDebugMarkerBegin = NULL;
    PFN_vkCmdDebugMarkerEndEXT          vkCmdDebugMarkerEnd = NULL;
    PFN_vkCmdDebugMarkerInsertEXT       vkCmdDebugMarkerInsert = NULL;
    PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectName = NULL;

    static bool s_bCanUseDebugMarker = false;

    bool ExtDebugMarkerCheckDeviceExtensions(DeviceProperties *pDP)
    {
        s_bCanUseDebugMarker = pDP->Add(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        return s_bCanUseDebugMarker;
    }

    //
    //
    void ExtDebugMarkersGetProcAddresses(VkDevice device)
    {
        if (s_bCanUseDebugMarker)
        {
            vkDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
            vkDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
            vkCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
            vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
            vkCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
            vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
        }
    }


    void SetPerfMarkerBegin(VkCommandBuffer cmd_buf, char *pMsg)
    {
        if (vkCmdDebugMarkerBegin)
        {
            VkDebugMarkerMarkerInfoEXT markerInfo = {};
            markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
            // Color to display this region with (if supported by debugger)
            float color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
            memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
            // Name of the region displayed by the debugging application
            markerInfo.pMarkerName = pMsg;
            vkCmdDebugMarkerBegin(cmd_buf, &markerInfo);
        }
    }

    void SetPerfMarkerEnd(VkCommandBuffer cmd_buf)
    {
        if (vkCmdDebugMarkerEnd)
        {
            vkCmdDebugMarkerEnd(cmd_buf);
        }
    }
}
