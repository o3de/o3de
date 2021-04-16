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
#include "../Base/DynamicBufferRing.h"
#include "../Base/StaticBufferPool.h"
#include "../Base/UploadHeap.h"
#include "../Base/Texture.h"
#include "../Base/Helper.h"
#include "PostProcPS.h"
#include "SkyDomeProc.h"

namespace CAULDRON_DX12
{
    void SkyDomeProc::OnCreate(
        Device* pDevice,
        ResourceViewHeaps *pResourceViewHeaps,
        DynamicBufferRing *pDynamicBufferRing,
        StaticBufferPool *pStaticBufferPool,
        DXGI_FORMAT outFormat,
        uint32_t sampleDescCount)
    {
        m_pDevice = pDevice;
        m_pDynamicBufferRing = pDynamicBufferRing;
        m_pResourceViewHeaps = pResourceViewHeaps;

        m_skydome.OnCreate(pDevice, "SkyDomeProc.hlsl", pResourceViewHeaps, pStaticBufferPool, 0, 0, NULL, outFormat, sampleDescCount);
    }

    void SkyDomeProc::OnDestroy()
    {
        m_skydome.OnDestroy();
    }

    void SkyDomeProc::Draw(ID3D12GraphicsCommandList* pCommandList, SkyDomeProc::Constants constants)
    {
        UserMarker marker(pCommandList, "Skydome Proc");

        SkyDomeProc::Constants *cbPerDraw;
        D3D12_GPU_VIRTUAL_ADDRESS constantBuffer;
        m_pDynamicBufferRing->AllocConstantBuffer(sizeof(SkyDomeProc::Constants), (void **)&cbPerDraw, &constantBuffer);
        *cbPerDraw = constants;

        m_skydome.Draw(pCommandList, 0, NULL, constantBuffer);
    }

    //
    // TODO: These functions below should generate a diffuse and specular cubemap to be used in IBL
    //
    /*
    void SkyDomeProc::GenerateDiffuseMapFromEnvironmentMap()
    {

    }

    void SkyDomeProc::CreateDiffCubeSRV(uint32_t index, VkDescriptorSet descriptorSet)
    {
        //SetDescriptor(m_pDevice->GetDevice(), index, m_CubeDiffuseTextureView, m_samplerDiffuseCube, descriptorSet);
    }

    void SkyDomeProc::CreateSpecCubeSRV(uint32_t index, VkDescriptorSet descriptorSet)
    {
        //SetDescriptor(m_pDevice->GetDevice(), index, m_CubeSpecularTextureView, m_samplerDiffuseCube, descriptorSet);
    }

    void SkyDomeProc::CreateBrdfSRV(uint32_t index, VkDescriptorSet descriptorSet)
    {
        //SetDescriptor(m_pDevice->GetDevice(), index, m_BrdfTextureView, m_samplerBDRF, descriptorSet);
    }
    */
}