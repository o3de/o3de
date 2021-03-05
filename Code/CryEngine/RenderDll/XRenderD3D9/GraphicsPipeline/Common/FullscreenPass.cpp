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

#include "RenderDll_precompiled.h"
#include "FullscreenPass.h"

#include "CryCustomTypes.h"
#include "DriverD3D.h"
#include "../Common/PostProcess/PostProcessUtils.h"

uint64 CFullscreenPass::s_prevRTMask = 0;

CFullscreenPass::CFullscreenPass()
{
    m_pShader = NULL;
    m_renderState = 0;
    m_rtMask = 0;
    m_dirtyMask = ~0;
    m_bRequireWPos = false;
    m_pRenderTargets.fill(NULL);
    m_vertexBuffer = ~0u;

    auto& factory = CDeviceObjectFactory::GetInstance();
    m_pResources = factory.CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
    m_pResourceLayout = factory.CreateResourceLayout();
}

CFullscreenPass::~CFullscreenPass()
{
    Reset();
}

void CFullscreenPass::Reset()
{
    m_ReflectedConstantBuffers.clear();
    m_dirtyMask = ~0;
}

void CFullscreenPass::BeginConstantUpdate()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    s_prevRTMask = rd->m_RP.m_FlagsShader_RT;
    rd->m_RP.m_FlagsShader_RT = m_rtMask;

    if (m_dirtyMask || m_pResources->IsDirty())
    {
        m_dirtyMask = CompileResources();
    }

    uint32 numPasses;
    m_pShader->FXSetTechnique(m_techniqueName);
    m_pShader->FXBegin(&numPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    SDeviceObjectHelpers::BeginUpdateConstantBuffers(m_ReflectedConstantBuffers);
}

void CFullscreenPass::Execute()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    

    // dummy PushRenderTarget here so we can directly set the target via the command list
    rd->FX_PushRenderTarget(0, m_pRenderTargets[0], NULL);

    // unmap constant buffers and mark as bound
    SDeviceObjectHelpers::EndUpdateConstantBuffers(m_ReflectedConstantBuffers);

    if (m_dirtyMask == 0)
    {
        // update vertex buffer if required
        if (m_bRequireWPos)
        {
            UpdateVertexBuffer();
        }

        size_t bufferOffset;
        uint32 stride = m_bRequireWPos ? sizeof(SVF_P3F_T2F_T3F) : sizeof(SVF_P3F_C4B_T2F);
        D3DBuffer* pVB = rd->m_DevBufMan.GetD3D(m_vertexBuffer, &bufferOffset);

        // fullscreen viewport
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(FullscreenPass_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
        #else
        D3DViewPort viewPort = { 0.0f, 0.0f, static_cast<float>(m_pRenderTargets[0]->GetWidth()), static_cast<float>(m_pRenderTargets[0]->GetHeight()), 0.f, 1.f };
        #endif
        D3D11_RECT viewPortRect = { (LONG)viewPort.TopLeftX, (LONG)viewPort.TopLeftY, LONG(viewPort.TopLeftX + viewPort.Width), LONG(viewPort.TopLeftY + viewPort.Height) };

        uint32 bindSlot = 0;
        CDeviceGraphicsCommandListPtr pCommandList = CDeviceObjectFactory::GetInstance().GetCoreGraphicsCommandList();

        pCommandList->SetRenderTargets(m_pRenderTargets.size(), &m_pRenderTargets[0], NULL);
        pCommandList->SetViewports(1, &viewPort);
        pCommandList->SetScissorRects(1, &viewPortRect);
        pCommandList->SetPipelineState(m_pPipelineState);
        pCommandList->SetResourceLayout(m_pResourceLayout.get());
        for (int i = 0; i < m_ReflectedConstantBuffers.size(); ++i)
        {
            pCommandList->SetInlineConstantBuffer(bindSlot++, m_ReflectedConstantBuffers[i].pBuffer, m_ReflectedConstantBuffers[i].shaderSlot, m_ReflectedConstantBuffers[i].shaderClass);
        }
        pCommandList->SetInlineConstantBuffer(bindSlot++, rd->GetGraphicsPipeline().GetPerViewConstantBuffer(), eConstantBufferShaderSlot_PerView, EShaderStage_Vertex | EShaderStage_Pixel);
        pCommandList->SetInlineConstantBuffer(bindSlot++, rd->GetGraphicsPipeline().GetPerFrameConstantBuffer(), eConstantBufferShaderSlot_PerFrame, EShaderStage_Vertex | EShaderStage_Pixel);
        pCommandList->SetResources(bindSlot++, m_pResources.get());
        pCommandList->SetVertexBuffers(1, &pVB, &bufferOffset, &stride);
        pCommandList->Draw(3, 1, 0, 0);
    }

    m_pShader->FXEndPass();
    m_pShader->FXEnd();

    rd->FX_PopRenderTarget(0);
    rd->m_RP.m_FlagsShader_RT = s_prevRTMask;
}

uint CFullscreenPass::CompileResources()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    // get required constant buffers
    bool shadersAvailable = SDeviceObjectHelpers::GetConstantBuffersFromShader(m_ReflectedConstantBuffers, m_pShader, m_techniqueName, m_rtMask, 0, 0);
    if (!shadersAvailable)
        return 0xFFFFFFFF;

    // textures
    m_pResources->Build();

    int bindSlot = 0;

    // resource mapping
    m_pResourceLayout->Clear();
    for (int i = 0; i < m_ReflectedConstantBuffers.size(); ++i)
    {
        m_pResourceLayout->SetConstantBuffer(bindSlot++, m_ReflectedConstantBuffers[i].shaderSlot, SHADERSTAGE_FROM_SHADERCLASS(m_ReflectedConstantBuffers[i].shaderClass));
    }
    m_pResourceLayout->SetConstantBuffer(bindSlot++, eConstantBufferShaderSlot_PerView, EShaderStage_Vertex | EShaderStage_Pixel);
    m_pResourceLayout->SetConstantBuffer(bindSlot++, eConstantBufferShaderSlot_PerFrame, EShaderStage_Vertex | EShaderStage_Pixel);
    m_pResourceLayout->SetResourceSet(bindSlot++, m_pResources);

    if (!m_pResourceLayout->Build())
    {
        return 0xFFFFFFFF;
    }

    // pipeline state
    CDeviceGraphicsPSODesc psoDesc(m_pResourceLayout.get(), m_pShader, m_techniqueName, m_rtMask, 0, 0, false);
    psoDesc.m_RenderState = m_renderState;
    psoDesc.m_VertexFormat = m_bRequireWPos ? eVF_P3F_T2F_T3F : eVF_P3F_C4B_T2F;
    psoDesc.m_PrimitiveType = eptTriangleStrip;
    for (int i = 0; i < m_pRenderTargets.size(); ++i)
    {
        psoDesc.m_RenderTargetFormats[i] = m_pRenderTargets[i] ? m_pRenderTargets[i]->GetDstFormat() : eTF_Unknown;
    }
    psoDesc.Build();

    m_pPipelineState = CDeviceObjectFactory::GetInstance().CreateGraphicsPSO(psoDesc);

    if (!m_pPipelineState)
    {
        return 0xFFFFFFFF;
    }

    // vertex buffer
    if (m_vertexBuffer != ~0u)
    {
        rd->m_DevBufMan.Destroy(m_vertexBuffer);
    }

    m_vertexBuffer = rd->m_DevBufMan.Create(BBT_VERTEX_BUFFER, m_bRequireWPos ? BU_DYNAMIC : BU_STATIC, 3 * (m_bRequireWPos ? sizeof(SVF_P3F_T2F_T3F) : sizeof(SVF_P3F_C4B_T2F)));
    UpdateVertexBuffer();

    return 0;
}

void CFullscreenPass::UpdateVertexBuffer()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    void* data = rd->m_DevBufMan.BeginWrite(m_vertexBuffer);

    if (m_bRequireWPos)
    {
        SVF_P3F_T2F_T3F result[3];
        SPostEffectsUtils::GetFullScreenTriWPOS(result, 0, 0);
        memcpy(data, result, sizeof(result));
    }
    else
    {
        SVF_P3F_C4B_T2F result[3];
        SPostEffectsUtils::GetFullScreenTri(result, 0, 0);
        memcpy(data, result, sizeof(result));
    }

    rd->m_DevBufMan.EndReadWrite(m_vertexBuffer);
}

