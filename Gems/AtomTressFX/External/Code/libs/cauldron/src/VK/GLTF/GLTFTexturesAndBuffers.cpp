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
#include "GltfHelpers.h"
#include "Base/UploadHeap.h"
#include "GLTFTexturesAndBuffers.h"
#include "Misc/Misc.h"
#include "Misc/ThreadPool.h"

namespace CAULDRON_VK
{
    bool GLTFTexturesAndBuffers::OnCreate(Device* pDevice, GLTFCommon *pGLTFCommon, UploadHeap* pUploadHeap, StaticBufferPool *pStaticBufferPool, DynamicBufferRing *pDynamicBufferRing)
    {
        m_pDevice = pDevice;
        m_pGLTFCommon = pGLTFCommon;
        m_pUploadHeap = pUploadHeap;
        m_pStaticBufferPool = pStaticBufferPool;
        m_pDynamicBufferRing = pDynamicBufferRing;

        return true;
    }

    void GLTFTexturesAndBuffers::LoadTextures()
    {
        // load textures and create views
        //
        if (m_pGLTFCommon->j3.find("images") != m_pGLTFCommon->j3.end())
        {
            m_pTextureNodes = m_pGLTFCommon->j3["textures"].get_ptr<const json::array_t *>();
            nlohmann::json::array_t images = m_pGLTFCommon->j3["images"];
            nlohmann::json::array_t materials = m_pGLTFCommon->j3["materials"];

            m_textures.resize(images.size());
            m_textureViews.resize(images.size());
            for (int i = 0; i < images.size(); i++)
            {
                // Identify what material uses this texture, this helps:
                // 1) determine the color space if the texture and also the cut out level. Authoring software saves albedo and emissive images in SRGB mode, the rest are linear mode
                // 2) tell the cutOff value, to prevent thinning of alpha tested PNGs when lower mips are used. 
                //
                bool useSRGB = false;
                float cutOff = 1.0f; // no cutoff
                for (int m = 0; m < materials.size(); m++)
                {
                    const json::object_t &material = materials[m].get_ref<const json::object_t &>();

                    if (GetElementInt(material, "pbrMetallicRoughness/baseColorTexture/index", -1) == i)
                    {
                        useSRGB = true;

                        cutOff = GetElementFloat(material, "alphaCutoff", 0.5);

                        break;
                    }

                    if (GetElementInt(material, "extensions/KHR_materials_pbrSpecularGlossiness/specularGlossinessTexture/index", -1) == i)
                    {
                        useSRGB = true;
                        break;
                    }

                    if (GetElementInt(material, "extensions/KHR_materials_pbrSpecularGlossiness/diffuseTexture/index", -1) == i)
                    {
                        useSRGB = true;
                        break;
                    }

                    if (GetElementInt(material, "emissiveTexture/index", -1) == i)
                    {
                        useSRGB = true;
                        break;
                    }
                }

                std::string filename = images[i]["uri"];
                bool result = m_textures[i].InitFromFile(m_pDevice, m_pUploadHeap, (m_pGLTFCommon->m_path + filename).c_str(), useSRGB, cutOff);
                assert(result != false);

                m_textures[i].CreateSRV(&m_textureViews[i]);
            }
            m_pUploadHeap->FlushAndFinish();
        }
    }

    void GLTFTexturesAndBuffers::OnDestroy()
    {
        for (int i = 0; i < m_textures.size(); i++)
        {
            vkDestroyImageView(m_pDevice->GetDevice(), m_textureViews[i], NULL);
            m_textures[i].OnDestroy();
        }
    }

    VkImageView GLTFTexturesAndBuffers::GetTextureViewByID(int id)
    {
        int tex = m_pTextureNodes->at(id)["source"];
        return m_textureViews[tex];
    }

    // Creates Vertex Buffers from accessors and sets them in the Primitive struct.
    //
    //
    void GLTFTexturesAndBuffers::CreateGeometry(tfAccessor indexBuffer, std::vector<tfAccessor> &vertexBuffers, Geometry *pGeometry)
    {
        pGeometry->m_NumIndices = indexBuffer.m_count;
        pGeometry->m_indexType = (indexBuffer.m_stride == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

        // Some exporters use 1-byte indices, need to convert them to shorts since the GPU doesn't support 1-byte indices
        if (indexBuffer.m_stride == 1)
        {
            unsigned short *pIndices = (unsigned short *)malloc(indexBuffer.m_count * (2 * indexBuffer.m_stride));
            for (int i = 0; i < indexBuffer.m_count; i++)
                pIndices[i] = ((unsigned char *)indexBuffer.m_data)[i];
            m_pStaticBufferPool->AllocBuffer(indexBuffer.m_count, 2 * indexBuffer.m_stride, pIndices, &pGeometry->m_IBV);
            free(pIndices);
        }
        else
        {
            m_pStaticBufferPool->AllocBuffer(indexBuffer.m_count, indexBuffer.m_stride, indexBuffer.m_data, &pGeometry->m_IBV);
        }

        // load the rest of the buffers onto the GPU
        pGeometry->m_VBV.resize(vertexBuffers.size());
        for (int i = 0; i < vertexBuffers.size(); i++)
        {
            tfAccessor *pVertexAccessor = &vertexBuffers[i];
            m_pStaticBufferPool->AllocBuffer(pVertexAccessor->m_count, pVertexAccessor->m_stride, pVertexAccessor->m_data, &pGeometry->m_VBV[i]);
        }
    }

    void GLTFTexturesAndBuffers::SetPerFrameConstants()
    {
        per_frame *cbPerFrame;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(per_frame), (void **)&cbPerFrame, &m_perFrameConstants);
        *cbPerFrame = m_pGLTFCommon->m_perFrameData;
    }

    void GLTFTexturesAndBuffers::SetSkinningMatricesForSkeletons()
    {
        for (auto &t : m_pGLTFCommon->m_pCurrentFrameTransformedData->m_worldSpaceSkeletonMats)
        {
            std::vector<XMMATRIX> *matrices = &t.second;

            VkDescriptorBufferInfo perSkeleton = {};
            XMMATRIX *cbPerSkeleton;
            m_pDynamicBufferRing->AllocConstantBuffer((uint32_t)(matrices->size() * sizeof(XMMATRIX)), (void **)&cbPerSkeleton, &perSkeleton);
            for (int i = 0; i < matrices->size(); i++)
            {
                cbPerSkeleton[i] = matrices->at(i);
            }

            m_skeletonMatricesBuffer[t.first] = perSkeleton;
        }
    }

    VkDescriptorBufferInfo *GLTFTexturesAndBuffers::GetSkinningMatricesBuffer(int skinIndex)
    {
        auto it = m_skeletonMatricesBuffer.find(skinIndex);

        if (it == m_skeletonMatricesBuffer.end())
            return NULL;

        return &it->second;
    }
}
