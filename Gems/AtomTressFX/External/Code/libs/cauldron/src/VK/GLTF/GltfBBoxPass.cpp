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
#include "Base/ExtDebugMarkers.h"
#include "GltfBBoxPass.h"
#include "GltfHelpers.h"
#include "Base/ShaderCompilerHelper.h"

namespace CAULDRON_VK
{
    //--------------------------------------------------------------------------------------
    //
    // OnCreate
    //
    //--------------------------------------------------------------------------------------
    void GltfBBoxPass::OnCreate(
        Device* pDevice,
        VkRenderPass renderPass,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        GLTFTexturesAndBuffers *pGLTFTexturesAndBuffers,
        VkSampleCountFlagBits sampleCount)
    {
        m_pGLTFTexturesAndBuffers = pGLTFTexturesAndBuffers;

        m_wireframeBox.OnCreate(pDevice, renderPass, pResourceViewHeaps, pDynamicBufferRing, pStaticBufferPool, sampleCount);
    }

    //--------------------------------------------------------------------------------------
    //
    // OnDestroy
    //
    //--------------------------------------------------------------------------------------
    void GltfBBoxPass::OnDestroy()
    {
        m_wireframeBox.OnDestroy();
    }

    //--------------------------------------------------------------------------------------
    //
    // Draw
    //
    //--------------------------------------------------------------------------------------
    void GltfBBoxPass::Draw(VkCommandBuffer cmd_buf, XMMATRIX cameraViewProjMatrix)
    {
        SetPerfMarkerBegin(cmd_buf, "bounding boxes");

        GLTFCommon *pC = m_pGLTFTexturesAndBuffers->m_pGLTFCommon;

        for (uint32_t i = 0; i < pC->m_nodes.size(); i++)
        {
            tfNode *pNode = &pC->m_nodes[i];
            if (pNode->meshIndex < 0)
                continue;

            XMMATRIX mWorldViewProj = pC->m_pCurrentFrameTransformedData->m_worldSpaceMats[i] * cameraViewProjMatrix;

            tfMesh *pMesh = &pC->m_meshes[pNode->meshIndex];
            for (uint32_t p = 0; p < pMesh->m_pPrimitives.size(); p++)
            {
                m_wireframeBox.Draw(cmd_buf, mWorldViewProj, pMesh->m_pPrimitives[p].m_center, pMesh->m_pPrimitives[p].m_radius, XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
            }
        }

        SetPerfMarkerEnd(cmd_buf);
    }
}
