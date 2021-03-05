/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once
#include <array>

class CFullscreenPass
{
public:
    CFullscreenPass();
    ~CFullscreenPass();

#define ASSIGN_VALUE(dst, src, dirtyFlag) \
    if ((dst) != (src)) {                 \
        m_dirtyMask |= (dirtyFlag); }     \
    (dst) = (src);

    void SetRenderTarget(uint32 slot, CTexture* pRenderTarget)
    {
        ASSIGN_VALUE(m_pRenderTargets[slot], pRenderTarget, 0xFFFFFFFF);
    }

    void SetTechnique(CShader* pShader, CCryNameTSCRC& techName, uint64 rtMask)
    {
        ASSIGN_VALUE(m_pShader, pShader, 0xFFFFFFFF);
        ASSIGN_VALUE(m_techniqueName, techName, 0xFFFFFFFF);
        ASSIGN_VALUE(m_rtMask, rtMask, 0xFFFFFFFF);
    }

    void SetTexture(uint32 slot, CTexture* pTexture, SResourceView::KeyType resourceViewID = SResourceView::DefaultView)
    {
        m_pResources->SetTexture(slot, pTexture, resourceViewID);
    }

    void SetSampler(uint32 slot, int32 sampler)
    {
        m_pResources->SetSampler(slot, sampler);
    }

    void SetTextureSamplerPair(uint32 slot, CTexture* pTex, int32 sampler, SResourceView::KeyType resourceViewID = SResourceView::DefaultView)
    {
        m_pResources->SetTexture(slot, pTex, resourceViewID);
        m_pResources->SetSampler(slot, sampler);
    }

    void SetState(int state)
    {
        ASSIGN_VALUE(m_renderState, state, 0xFFFFFFFF);
    }

    void SetRequireWorldPos(bool bRequireWPos)
    {
        ASSIGN_VALUE(m_bRequireWPos, bRequireWPos, 0xFFFFFFFF);
    }

#undef ASSIGN_VALUE

    void BeginConstantUpdate();
    void Execute();
    void Reset();

private:
    typedef SDeviceObjectHelpers::SConstantBufferBindInfo CBufferBindInfo;

    uint CompileResources();
    void UpdateVertexBuffer();

    std::array<CTexture*, 1>     m_pRenderTargets;
    CDeviceResourceSetPtr        m_pResources;
    std::vector<CBufferBindInfo> m_ReflectedConstantBuffers;
    CDeviceResourceLayoutPtr     m_pResourceLayout;
    CDeviceGraphicsPSOPtr        m_pPipelineState;
    CShader*                     m_pShader;
    CCryNameTSCRC                m_techniqueName;
    uint64                       m_rtMask;
    int                          m_renderState;
    uint                         m_dirtyMask;
    bool                         m_bRequireWPos;
    buffer_handle_t              m_vertexBuffer;

    static uint64  s_prevRTMask;
};

