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

#include "GLTFTexturesAndBuffers.h"

namespace CAULDRON_VK
{
    // Material, primitive and mesh structs specific for the depth pass (you might want to compare these structs with the ones used for the depth pass in GltfPbrPass.h)

    struct DepthMaterial
    {
        int m_textureCount = 0;
        VkDescriptorSet m_descriptorSet;
        VkDescriptorSetLayout m_descriptorSetLayout;

        DefineList m_defines;
        bool m_doubleSided = false;
    };

    struct DepthPrimitives
    {
        Geometry m_Geometry;

        DepthMaterial *m_pMaterial = NULL;

        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;

        VkDescriptorSet m_descriptorSet;
        VkDescriptorSetLayout m_descriptorSetLayout;
    };

    struct DepthMesh
    {
        std::vector<DepthPrimitives> m_pPrimitives;
    };

    class GltfDepthPass
    {
    public:
        struct per_frame
        {
            XMMATRIX mViewProj;
        };

        struct per_object
        {
            XMMATRIX mWorld;
        };

        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            UploadHeap* pUploadHeap,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers);

        void OnDestroy();
        GltfDepthPass::per_frame *SetPerFrameConstants();
        void Draw(VkCommandBuffer cmd_buf);
    private:
        ResourceViewHeaps *m_pResourceViewHeaps;
        DynamicBufferRing *m_pDynamicBufferRing;
        StaticBufferPool *m_pStaticBufferPool;

        std::vector<DepthMesh> m_meshes;
        std::vector<DepthMaterial> m_materialsData;

        DepthMaterial m_defaultMaterial;

        Device* m_pDevice;
        GLTFTexturesAndBuffers *m_pGLTFTexturesAndBuffers;
        VkRenderPass m_renderPass;
        VkSampler m_sampler;
        VkDescriptorBufferInfo m_perFrameDesc;

        void CreateDescriptors(Device* pDevice, int inverseMatrixBufferSize, DefineList *pAttributeDefines, DepthPrimitives *pPrimitive);
        void CreatePipeline(Device* pDevice, std::vector<VkVertexInputAttributeDescription> layout, DefineList *pAttributeDefines, DepthPrimitives *pPrimitive);
    };
}


