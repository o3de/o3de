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

namespace CAULDRON_VK
{
    // This class will create a Descriptor Pool and allows allocating, freeing and initializing Descriptor Set Layouts(DSL) from this pool. 

    class ResourceViewHeaps
    {
    public:
        void OnCreate(Device *pDevice, uint32_t cbvDescriptorCount, uint32_t srvDescriptorCount, uint32_t uavDescriptorCount, uint32_t samplerDescriptorCount);
        void OnDestroy();
        bool AllocDescriptor(VkDescriptorSetLayout descriptorLayout, VkDescriptorSet *pDescriptor);
        bool AllocDescriptor(int size, const VkSampler *pSamplers, VkDescriptorSetLayout *descriptorLayout, VkDescriptorSet *pDescriptor);
        bool CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding, VkDescriptorSetLayout *pDescSetLayout);
        bool CreateDescriptorSetLayoutAndAllocDescriptorSet(std::vector<VkDescriptorSetLayoutBinding> *pDescriptorLayoutBinding, VkDescriptorSetLayout *descriptorLayout, VkDescriptorSet *pDescriptor);
        void FreeDescriptor(VkDescriptorSet descriptorSet);
    private:
        Device          *m_pDevice;
        VkDescriptorPool m_descriptorPool;
        std::mutex       m_mutex;
        int              m_allocatedDescriptorCount = 0;
    };
}

