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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DEVICEMANAGER_DEVICEMANAGERINLINE_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DEVICEMANAGER_DEVICEMANAGERINLINE_H
#pragma once


#if !defined(NULL_RENDERER) && DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
#include "../DriverD3D.h"
#endif

//===============================================================================================================
//
//  Inline implementation for CDeviceManager
//
//===============================================================================================================

#if !defined(NULL_RENDERER)
inline void CDeviceManager::BindSRV(EHWShaderClass type, D3DShaderResourceView* SRV, uint32 slot)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    switch (type)
    {
    case eHWSC_Vertex:
        rDeviceContext.VSSetShaderResources(slot, 1, &SRV);
        break;
    case eHWSC_Pixel:
        rDeviceContext.PSSetShaderResources(slot, 1, &SRV);
        break;
    case eHWSC_Geometry:
        rDeviceContext.GSSetShaderResources(slot, 1, &SRV);
        break;
    case eHWSC_Domain:
        rDeviceContext.DSSetShaderResources(slot, 1, &SRV);
        break;
    case eHWSC_Hull:
        rDeviceContext.HSSetShaderResources(slot, 1, &SRV);
        break;
    case eHWSC_Compute:
        rDeviceContext.CSSetShaderResources(slot, 1, &SRV);
        break;
    }
#else
    const unsigned dirty_base = slot >> SRV_DIRTY_SHIFT;
    const unsigned dirty_bit  = slot & SRV_DIRTY_MASK;
    m_SRV[type].views[slot] = SRV;
    m_SRV[type].dirty[dirty_base] |= 1 << dirty_bit;
#endif
}
inline void CDeviceManager::BindSRV(EHWShaderClass type, D3DShaderResourceView** SRV, uint32 start_slot, uint32 count)
{
    for (unsigned i = 0; i < count; ++i)
    {
        BindSRV(type, SRV[i], i + start_slot);
    }
}
inline void CDeviceManager::BindUAV(EHWShaderClass type, D3DUnorderedAccessView* UAV, uint32 count, uint32 slot)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    switch (type)
    {
    case eHWSC_Vertex:
        assert(0 && "NOT IMPLEMENTED ON D3D11.0");
        break;
    case eHWSC_Pixel:
        rDeviceContext.OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, slot, 1, &UAV, &count);
        break;
    case eHWSC_Geometry:
        assert(0 && "NOT IMPLEMENTED ON D3D11.0");
        break;
    case eHWSC_Domain:
        assert(0 && "NOT IMPLEMENTED ON D3D11.0");
        break;
    case eHWSC_Hull:
        assert(0 && "NOT IMPLEMENTED ON D3D11.0");
        break;
    case eHWSC_Compute:
        rDeviceContext.CSSetUnorderedAccessViews(slot, 1, &UAV, &count);
        break;
    }
#else
    const unsigned dirty_base = slot >> UAV_DIRTY_SHIFT;
    const unsigned dirty_bit  = slot & UAV_DIRTY_MASK;
    m_UAV[type].views[slot] = UAV;
    m_UAV[type].counts[slot] = count;
    m_UAV[type].dirty[dirty_base] |= 1 << dirty_bit;
#endif
}
inline void CDeviceManager::BindUAV(EHWShaderClass type, D3DUnorderedAccessView** UAV, const uint32* counts, uint32 start_slot, uint32 count)
{
    for (unsigned i = 0; i < count; ++i)
    {
        BindUAV(type, UAV[i], (counts ? counts[i] : -1), i + start_slot);
    }
}
inline void CDeviceManager::BindSampler(EHWShaderClass type, D3DSamplerState* Sampler, uint32 slot)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    switch (type)
    {
    case eHWSC_Vertex:
        rDeviceContext.VSSetSamplers(slot, 1, &Sampler);
        break;
    case eHWSC_Pixel:
        rDeviceContext.PSSetSamplers(slot, 1, &Sampler);
        break;
    case eHWSC_Geometry:
        rDeviceContext.GSSetSamplers(slot, 1, &Sampler);
        break;
    case eHWSC_Domain:
        rDeviceContext.DSSetSamplers(slot, 1, &Sampler);
        break;
    case eHWSC_Hull:
        rDeviceContext.HSSetSamplers(slot, 1, &Sampler);
        break;
    case eHWSC_Compute:
        rDeviceContext.CSSetSamplers(slot, 1, &Sampler);
        break;
    }
#else
    if (Sampler != m_Samplers[type].samplers[slot])
    {
        m_Samplers[type].samplers[slot] = Sampler;
        m_Samplers[type].dirty |= 1 << slot;
    }
#endif
}
inline void CDeviceManager::BindSampler(EHWShaderClass type, D3DSamplerState** Samplers, uint32 start_slot, uint32 count)
{
    for (unsigned i = 0; i < count; ++i)
    {
        BindSampler(type, Samplers[i], i + start_slot);
    }
}
inline void CDeviceManager::BindVB(D3DBuffer* Buffer, uint32 slot, uint32 offset, uint32 stride)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    rDeviceContext.IASetVertexBuffers(slot, 1, &Buffer, &stride, &offset);
#else
    m_VBs.buffers[slot] = Buffer;
    m_VBs.offsets[slot] = offset;
    m_VBs.strides[slot] = stride;
    m_VBs.dirty |= 1 << slot;
#endif
}
inline void CDeviceManager::BindVB(uint32 start, uint32 count, D3DBuffer** Buffers, uint32* offset, uint32* stride)
{
    for (size_t i = 0; i < count; ++i)
    {
        BindVB(Buffers[i], start + i, offset[i], stride[i]);
    }
}
inline void CDeviceManager::BindIB(D3DBuffer* Buffer, uint32 offset, DXGI_FORMAT format)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    rDeviceContext.IASetIndexBuffer(Buffer, format, offset);
#else
    m_IB.buffer = Buffer;
    m_IB.offset = offset;
    m_IB.format = format;
    m_IB.dirty = 1;
#endif
}
inline void CDeviceManager::BindVtxDecl(D3DVertexDeclaration* decl)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    rDeviceContext.IASetInputLayout(decl);
#else
    m_VertexDecl.decl = decl;
    m_VertexDecl.dirty = true;
#endif
}
inline void CDeviceManager::BindTopology(D3D11_PRIMITIVE_TOPOLOGY top)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    rDeviceContext.IASetPrimitiveTopology(top);
#else
    m_Topology.topology = top;
    m_Topology.dirty = true;
#endif
}
inline void CDeviceManager::BindShader(EHWShaderClass type, ID3D11Resource* shader)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    switch (type)
    {
    case eHWSC_Vertex:
        rDeviceContext.VSSetShader((D3DVertexShader*)shader, NULL, 0);
        break;
    case eHWSC_Pixel:
        rDeviceContext.PSSetShader((D3DPixelShader*)shader, NULL, 0);
        break;
    case eHWSC_Hull:
        rDeviceContext.HSSetShader((ID3D11HullShader*)shader, NULL, 0);
        break;
    case eHWSC_Geometry:
        rDeviceContext.GSSetShader((ID3D11GeometryShader*)shader, NULL, 0);
        break;
    case eHWSC_Domain:
        rDeviceContext.DSSetShader((ID3D11DomainShader*)shader, NULL, 0);
        break;
    case eHWSC_Compute:
        rDeviceContext.CSSetShader((ID3D11ComputeShader*)shader, NULL, 0);
        break;
    }
#else
    m_Shaders[type].shader = shader;
    m_Shaders[type].dirty = true;
#endif
}
inline void CDeviceManager::SetDepthStencilState(ID3D11DepthStencilState* dss, uint32 stencilref)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    rDeviceContext.OMSetDepthStencilState(dss, stencilref);
#else
    m_DepthStencilState.dss = dss;
    m_DepthStencilState.stencilref = stencilref;
    m_DepthStencilState.dirty = true;
#endif
}
inline void CDeviceManager::SetBlendState(ID3D11BlendState* pBlendState, float* BlendFactor, uint32 SampleMask)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    rDeviceContext.OMSetBlendState(pBlendState, BlendFactor, SampleMask);
#else
    m_BlendState.pBlendState = pBlendState;
    if (BlendFactor)
    {
        for (size_t i = 0; i < 4; ++i)
        {
            m_BlendState.BlendFactor[i] = BlendFactor[i];
        }
    }
    else
    {
        for (size_t i = 0; i < 4; ++i)
        {
            m_BlendState.BlendFactor[i] = 1.f;
        }
    }
    m_BlendState.SampleMask = SampleMask;
    m_BlendState.dirty = true;
#endif
}
inline void CDeviceManager::SetRasterState(ID3D11RasterizerState* pRasterizerState)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
    D3DDeviceContext& rDeviceContext = gcpRendD3D->GetDeviceContext();
    rDeviceContext.RSSetState(pRasterizerState);
#else
    m_RasterState.pRasterizerState = pRasterizerState;
    m_RasterState.dirty = true;
#endif
}
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
ILINE void CDeviceManager::CommitDeviceStates()
{
    // Nothing to do for DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
}
#endif
#endif
//====================================================================================================

#endif  // _DeviceManager_H_
