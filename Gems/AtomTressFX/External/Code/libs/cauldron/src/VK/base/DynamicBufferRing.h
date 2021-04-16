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
#include "Misc/Ring.h"
#include "../VulkanMemoryAllocator/vk_mem_alloc.h"

namespace CAULDRON_VK
{
    // This class mimics the behaviour or the DX11 dynamic buffers. I can hold uniforms, index and vertex buffers.
    // It does so by suballocating memory from a huge buffer. The buffer is used in a ring fashion.  
    // Allocated memory is taken from the tail, freed memory makes the head advance;
    // See 'ring.h' to get more details on the ring buffer.
    //
    // The class knows when to free memory by just knowing:
    //    1) the amount of memory used per frame
    //    2) the number of backbuffers 
    //    3) When a new frame just started ( indicated by OnBeginFrame() )
    //         - This will free the data of the oldest frame so it can be reused for the new frame
    //
    // Note than in this ring an allocated chuck of memory has to be contiguous in memory, that is it cannot spawn accross the tail and the head.
    // This class takes care of that.

    class DynamicBufferRing
    {
    public:
        VkResult OnCreate(Device *pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize, char *name = NULL);
        void OnDestroy();
        bool AllocConstantBuffer(uint32_t size, void **pData, VkDescriptorBufferInfo *pOut);
        bool AllocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void **pData, VkDescriptorBufferInfo *pOut);
        bool AllocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void **pData, VkDescriptorBufferInfo *pOut);
        void OnBeginFrame();
        void SetDescriptorSet(int i, uint32_t size, VkDescriptorSet descriptorSet);

    private:
        Device         *m_pDevice;
        uint32_t        m_memTotalSize;
        RingWithTabs    m_mem;
        char           *m_pData = nullptr;
        VkBuffer        m_buffer;

#ifdef USE_VMA
        VmaAllocation   m_bufferAlloc = VK_NULL_HANDLE;
#else
        VkDeviceMemory  m_deviceMemory = VK_NULL_HANDLE;
#endif
    };
}