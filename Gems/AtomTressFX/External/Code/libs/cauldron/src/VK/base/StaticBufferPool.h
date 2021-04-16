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

#include "Device.h"
#include "Base/ResourceViewHeaps.h"
#include "../VulkanMemoryAllocator/vk_mem_alloc.h"

namespace CAULDRON_VK
{
    // Simulates DX11 style static buffers. For dynamic buffers please see 'DynamicBufferRingDX12.h'
    //
    // This class allows suballocating small chuncks of memory from a huge buffer that is allocated on creation 
    // This class is specialized in vertex and index buffers. 
    //
    class StaticBufferPool
    {
    public:
        VkResult OnCreate(Device *pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name);
        void OnDestroy();

        // Allocates a IB/VB and returns a pointer to fill it + a descriptot
        //
        bool AllocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void **pData, VkDescriptorBufferInfo *pOut);

        // Allocates a IB/VB and fill it with pInitData, returns a descriptor
        //
        bool AllocBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void *pInitData, VkDescriptorBufferInfo *pOut);

        // if using vidmem this kicks the upload from the upload heap to the video mem
        void UploadData(VkCommandBuffer cmd_buf);

        // if using vidmem frees the upload heap
        void FreeUploadHeap();

    private:
        Device          *m_pDevice;

        std::mutex       m_mutex = {};

        bool             m_bUseVidMem = true;

        char            *m_pData = nullptr;
        uint32_t         m_memOffset = 0;
        uint32_t         m_totalMemSize = 0;

        VkBuffer         m_buffer;
        VkBuffer         m_bufferVid;

#ifdef USE_VMA
        VmaAllocation    m_bufferAlloc = VK_NULL_HANDLE;
        VmaAllocation    m_bufferAllocVid = VK_NULL_HANDLE;
#else
        VkDeviceMemory   m_deviceMemory = VK_NULL_HANDLE;;
        VkDeviceMemory   m_deviceMemoryVid = VK_NULL_HANDLE;;
#endif
    };
}

