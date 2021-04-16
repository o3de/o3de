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
#include "PostProc/SkyDome.h"
#include "../common/GLTF/GltfPbrMaterial.h"

namespace CAULDRON_VK
{
    // Material, primitive and mesh structs specific for the PBR pass (you might want to compare these structs with the ones used for the depth pass in GltfDepthPass.h)

    struct PBRMaterial
    {
        int m_textureCount = 0;
        VkDescriptorSet m_texturesDescriptorSet;
        VkDescriptorSetLayout m_descriptorLayout;

        PBRMaterialParameters m_pbrMaterialParameters;
    };

    struct PBRPrimitives
    {
        Geometry m_geometry;

        PBRMaterial *m_pMaterial = NULL;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout;

        VkDescriptorSet m_descriptorSet;
        VkDescriptorSetLayout m_descriptorLayout;

        void DrawPrimitive(VkCommandBuffer cmd_buf, VkDescriptorBufferInfo perSceneDesc, VkDescriptorBufferInfo perObjectDesc, VkDescriptorBufferInfo *pPerSkeleton);
    };

    struct PBRMesh
    {
        std::vector<PBRPrimitives> m_pPrimitives;
    };

    class GltfPbrPass
    {
    public:
        struct per_object
        {
            XMMATRIX mWorld;

            PBRMaterialParametersConstantBuffer m_pbrParams;
        };

        void OnCreate(
            Device* pDevice,
            VkRenderPass renderPass,
            UploadHeap* pUploadHeap,
            ResourceViewHeaps *pHeaps,
            DynamicBufferRing *pDynamicBufferRing,
            StaticBufferPool *pStaticBufferPool,
            GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
            SkyDome *pSkyDome,
            VkImageView ShadowMapView,
            VkSampleCountFlagBits sampleCount
        );

        void OnDestroy();
        void Draw(VkCommandBuffer cmd_buf);
    private:
        GLTFTexturesAndBuffers *m_pGLTFTexturesAndBuffers;

        ResourceViewHeaps *m_pResourceViewHeaps;
        DynamicBufferRing *m_pDynamicBufferRing;
        StaticBufferPool *m_pStaticBufferPool;

        std::vector<PBRMesh> m_meshes;
        std::vector<PBRMaterial> m_materialsData;

        GltfPbrPass::per_frame m_cbPerFrame;

        PBRMaterial m_defaultMaterial;

        Device* m_pDevice;
        VkRenderPass m_renderPass;
        VkSampleCountFlagBits m_sampleCount;
        VkSampler m_samplerPbr, m_samplerShadow;

        // PBR Brdf
        Texture m_brdfLutTexture;
        VkImageView m_brdfLutView;
        VkSampler m_brdfLutSampler;

        void CreateGPUMaterialData(PBRMaterial *tfmat, std::map<std::string, VkImageView> &texturesBase, SkyDome *pSkyDome, VkImageView ShadowMapView);
        void CreateDescriptors(Device *pDevice, int inverseMatrixBufferSize, DefineList *pAttributeDefines, PBRPrimitives *pPrimitive);
        void CreatePipeline(Device *pDevice, std::vector<VkVertexInputAttributeDescription> layout, DefineList *pAttributeDefines, PBRPrimitives *pPrimitive);
    };
}


