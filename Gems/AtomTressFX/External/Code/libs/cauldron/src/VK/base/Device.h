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
#pragma once

#include "vulkan/vulkan.h"


#define USE_VMA

#ifdef USE_VMA
#include "../VulkanMemoryAllocator/vk_mem_alloc.h"
#endif

namespace CAULDRON_VK
{
    //
    // Just a device class with many helper functions
    //

    class Device
    {
    public:
        Device();
        ~Device();
        void OnCreate(const char *pAppName, const char *pEngine, bool bValidationEnabled, HWND hWnd);
        void OnDestroy();
        VkDevice GetDevice() { return m_device; }
        VkQueue GetGraphicsQueue() { return graphics_queue; }
        uint32_t GetGraphicsQueueFamilyIndex() { return present_queue_family_index; }
        VkQueue GetPresentQueue() { return present_queue; }
        uint32_t GetPresentQueueFamilyIndex() { return graphics_queue_family_index; }
        VkQueue GetComputeQueue() { return compute_queue; }
        uint32_t GetComputeQueueFamilyIndex() { return compute_queue_family_index; }
        VkPhysicalDevice GetPhysicalDevice() { return m_physicaldevice; }
        VkSurfaceKHR GetSurface() { return m_surface; }
#ifdef USE_VMA
        VmaAllocator GetAllocator() { return m_hAllocator; }
#endif
        VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() { return m_memoryProperties; }
        VkPhysicalDeviceProperties GetPhysicalDeviceProperries() { return m_deviceProperties; }

        bool IsFp16Supported() { return m_usingFp16; };

        // pipeline cache
        VkPipelineCache m_pipelineCache;
        void CreatePipelineCache();
        void DestroyPipelineCache();
        VkPipelineCache GetPipelineCache();

        void CreateShaderCache() {};
        void DestroyShaderCache() {};

        void GPUFlush();

    private:
        VkInstance m_instance;
        VkDevice m_device;
        VkPhysicalDevice m_physicaldevice;
        VkPhysicalDeviceMemoryProperties m_memoryProperties;
        VkPhysicalDeviceProperties m_deviceProperties;
        VkSurfaceKHR m_surface;

        VkQueue present_queue;
        uint32_t present_queue_family_index;
        VkQueue graphics_queue;
        uint32_t graphics_queue_family_index;
        VkQueue compute_queue;
        uint32_t compute_queue_family_index;

        bool m_usingValidationLayer = false;
        bool m_usingFp16 = false;
#ifdef USE_VMA
        VmaAllocator m_hAllocator = NULL;
#endif
    };

    bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties &memory_properties, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
}
