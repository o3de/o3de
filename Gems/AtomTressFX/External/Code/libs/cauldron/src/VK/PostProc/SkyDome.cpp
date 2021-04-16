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
#include "Base/DynamicBufferRing.h"
#include "Base/StaticBufferPool.h"
#include "Base/ExtDebugMarkers.h"
#include "Base/UploadHeap.h"
#include "Base/Helper.h"
#include "Base/Texture.h"
#include "SkyDome.h"

namespace CAULDRON_VK
{
    void SkyDome::OnCreate(Device* pDevice, VkRenderPass renderPass, UploadHeap* pUploadHeap, VkFormat outFormat, ResourceViewHeaps *pResourceViewHeaps, DynamicBufferRing *pDynamicBufferRing, StaticBufferPool  *pStaticBufferPool, char *pDiffuseCubemap, char *pSpecularCubemap, VkSampleCountFlagBits sampleDescCount)
    {
        m_pDevice = pDevice;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pResourceViewHeaps = pResourceViewHeaps;

        m_CubeDiffuseTexture.InitFromFile(pDevice, pUploadHeap, pDiffuseCubemap, true); // SRGB
        m_CubeSpecularTexture.InitFromFile(pDevice, pUploadHeap, pSpecularCubemap, true);
        pUploadHeap->FlushAndFinish();

        m_CubeDiffuseTexture.CreateCubeSRV(&m_CubeDiffuseTextureView);
        m_CubeSpecularTexture.CreateCubeSRV(&m_CubeSpecularTextureView);        

        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_NEAREST;
            info.minFilter = VK_FILTER_NEAREST;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplerDiffuseCube);
            assert(res == VK_SUCCESS);
        }

        {
            VkSamplerCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            info.minLod = -1000;
            info.maxLod = 1000;
            info.maxAnisotropy = 1.0f;
            VkResult res = vkCreateSampler(pDevice->GetDevice(), &info, NULL, &m_samplerSpecularCube);
            assert(res == VK_SUCCESS);
        }

        //create descriptor
        //
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings(2);
        layout_bindings[0].binding = 0;
        layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout_bindings[0].descriptorCount = 1;
        layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[0].pImmutableSamplers = NULL;

        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[1].pImmutableSamplers = NULL;

        m_pResourceViewHeaps->CreateDescriptorSetLayoutAndAllocDescriptorSet(&layout_bindings, &m_descriptorLayout, &m_descriptorSet);
        pDynamicBufferRing->SetDescriptorSet(0, sizeof(XMMATRIX), m_descriptorSet);
        SetDescriptorSpec(1, m_descriptorSet);

        m_skydome.OnCreate(pDevice, renderPass, "SkyDome.glsl", pStaticBufferPool, pDynamicBufferRing, m_descriptorLayout, NULL, sampleDescCount);
    }

    void SkyDome::OnDestroy()
    {
        m_skydome.OnDestroy();
        vkDestroyDescriptorSetLayout(m_pDevice->GetDevice(), m_descriptorLayout, NULL);

        vkDestroySampler(m_pDevice->GetDevice(), m_samplerDiffuseCube, nullptr);
        vkDestroySampler(m_pDevice->GetDevice(), m_samplerSpecularCube, nullptr);

        vkDestroyImageView(m_pDevice->GetDevice(), m_CubeDiffuseTextureView, NULL);
        vkDestroyImageView(m_pDevice->GetDevice(), m_CubeSpecularTextureView, NULL);

        m_pResourceViewHeaps->FreeDescriptor(m_descriptorSet);

        m_CubeDiffuseTexture.OnDestroy();
        m_CubeSpecularTexture.OnDestroy();
    }

    void SkyDome::SetDescriptorDiff(uint32_t index, VkDescriptorSet descriptorSet)
    {
        SetDescriptorSet(m_pDevice->GetDevice(), index, m_CubeDiffuseTextureView, &m_samplerDiffuseCube, descriptorSet);
    }

    void SkyDome::SetDescriptorSpec(uint32_t index, VkDescriptorSet descriptorSet)
    {
        SetDescriptorSet(m_pDevice->GetDevice(), index, m_CubeSpecularTextureView, &m_samplerSpecularCube, descriptorSet);
    }

    void SkyDome::Draw(VkCommandBuffer cmd_buf, XMMATRIX invViewProj)
    {
        SetPerfMarkerBegin(cmd_buf, "Skydome cube");

        XMMATRIX *cbPerDraw;
        VkDescriptorBufferInfo constantBuffer;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(XMMATRIX), (void **)&cbPerDraw, &constantBuffer);
        *cbPerDraw = invViewProj;

        m_skydome.Draw(cmd_buf, constantBuffer, m_descriptorSet);

        SetPerfMarkerEnd(cmd_buf);
    }


    void SkyDome::GenerateDiffuseMapFromEnvironmentMap()
    {

    }
}
