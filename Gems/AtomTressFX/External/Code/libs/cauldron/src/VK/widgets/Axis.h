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

#include "Base/ResourceViewHeaps.h"
#include "Base/DynamicBufferRing.h"
#include "Base/StaticBufferPool.h"

namespace CAULDRON_VK
{
    // This class takes a GltfCommon class (that holds all the non-GPU specific data) as an input and loads all the GPU specific data
    //
    class Axis
    {
    public:
        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            VkSampleCountFlagBits sampleDescCount);

        void OnDestroy();
        void Draw(VkCommandBuffer cmd_buf, XMMATRIX worldMatrix, XMMATRIX axisMatrix);
    private:

        Device* m_pDevice;

        DynamicBufferRing *m_pDynamicBufferRing;
        StaticBufferPool *m_pStaticBufferPool;
        ResourceViewHeaps *m_pResourceViewHeaps;

        // all bounding boxes of all the meshes use the same geometry, shaders and pipelines.
        uint32_t m_NumIndices;
        VkIndexType m_indexType;
        VkDescriptorBufferInfo m_IBV;
        VkDescriptorBufferInfo m_VBV;

        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;

        VkDescriptorSet             m_descriptorSet;
        VkDescriptorSetLayout       m_descriptorSetLayout;

        struct per_object
        {
            XMMATRIX m_mWorldViewProj;
        };
    };
}

