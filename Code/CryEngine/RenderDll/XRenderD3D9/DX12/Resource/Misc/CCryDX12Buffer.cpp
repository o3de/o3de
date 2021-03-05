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
#include "CCryDX12Buffer.hpp"
#include "../GI/CCryDX12SwapChain.hpp"

CCryDX12Buffer* CCryDX12Buffer::Create(CCryDX12Device* pDevice, ID3D12Resource* pResource, D3D12_RESOURCE_STATES initialState)
{
    DX12_ASSERT(pResource);

    D3D12_RESOURCE_DESC desc12 = pResource->GetDesc();

    D3D11_BUFFER_DESC desc11;
    desc11.ByteWidth = 0; // static_cast<UINT>(desc12.ByteWidth);
    desc11.StructureByteStride = 0; // static_cast<UINT>(desc12.StructureByteStride);
    desc11.Usage = D3D11_USAGE_DEFAULT;
    desc11.CPUAccessFlags = 0;
    desc11.BindFlags = 0;
    desc11.MiscFlags = 0;

    if (desc12.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        desc11.BindFlags |= D3D11_BIND_RENDER_TARGET;
    }

    if (desc12.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        desc11.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }

    if (desc12.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
        desc11.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    CCryDX12Buffer* result = DX12::PassAddRef(new CCryDX12Buffer(pDevice, desc11, pResource, initialState, CD3DX12_RESOURCE_DESC(desc12), NULL));

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Buffer* CCryDX12Buffer::Create(CCryDX12Device* pDevice, CCryDX12SwapChain* pSwapChain, ID3D12Resource* pResource)
{
    CCryDX12Buffer* result = Create(pDevice, pResource, D3D12_RESOURCE_STATE_RENDER_TARGET);
    result->GetDX12Resource().SetDX12SwapChain(pSwapChain->GetDX12SwapChain());

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Buffer* CCryDX12Buffer::Create(CCryDX12Device* pDevice, const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData)
{
    DX12_ASSERT(pDesc, "CreateBuffer() called without description!");

    D3D12_RESOURCE_DESC desc12 = CD3DX12_RESOURCE_DESC::Buffer((pDesc->ByteWidth + 255) & ~255);

    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_STATES resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D11_SUBRESOURCE_DATA sInitialData;
    if (pInitialData)
    {
        sInitialData = *pInitialData;
        sInitialData.SysMemPitch = pDesc->ByteWidth;
        sInitialData.SysMemSlicePitch = pDesc->ByteWidth;
        pInitialData = &sInitialData;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
    {
        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    else if (pDesc->Usage == D3D11_USAGE_STAGING)
    {
        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    else if (pDesc->Usage == D3D11_USAGE_DYNAMIC)
    {
        heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        resourceUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    if (pDesc->CPUAccessFlags != 0)
    {
        // TODO: Check feasibility and performance of allocating staging textures as row major buffers
        // and using CopyTextureRegion instead of ReadFromSubresource/WriteToSubresource

        if (pDesc->CPUAccessFlags == D3D11_CPU_ACCESS_WRITE)
        {
            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            resourceUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else if (pDesc->CPUAccessFlags == D3D11_CPU_ACCESS_READ)
        {
            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
            resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
        }
        else
        {
            DX12_NOT_IMPLEMENTED;
            return nullptr;
        }
    }

    if (pDesc->BindFlags & D3D11_BIND_UNORDERED_ACCESS)
    {
        desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        resourceUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    if (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        resourceUsage = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    if (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)
    {
        desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        resourceUsage = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    ID3D12Resource* resource = NULL;

    HRESULT hr = pDevice->GetD3D12Device()->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &desc12,
        resourceUsage,
        NULL,
        IID_GRAPHICS_PPV_ARGS(&resource));

    if (hr != S_OK || !resource)
    {
        DX12_ASSERT(0, "Could not create buffer resource!");
        return NULL;
    }

    auto result = DX12::PassAddRef(new CCryDX12Buffer(pDevice, *pDesc, resource, resourceUsage, CD3DX12_RESOURCE_DESC(resource->GetDesc()), pInitialData, 1));
    resource->Release();

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Buffer::CCryDX12Buffer(CCryDX12Device* pDevice, const D3D11_BUFFER_DESC& desc11, ID3D12Resource* pResource, D3D12_RESOURCE_STATES eInitialState, const D3D12_RESOURCE_DESC& desc12, const D3D11_SUBRESOURCE_DATA* pInitialData, size_t numInitialData)
    : Super(pDevice, pResource, eInitialState, desc12, pInitialData, numInitialData)
    , m_Desc11(desc11)
{
    m_Desc11.StructureByteStride = (m_Desc11.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) ? m_Desc11.StructureByteStride : 0U;
    m_DX12View.Init(m_DX12Resource, DX12::EVT_ConstantBufferView, m_Desc11.ByteWidth);
}


CCryDX12Buffer::~CCryDX12Buffer()
{
}

CCryDX12Buffer* CCryDX12Buffer::AcquireUploadBuffer()
{
    if (m_UploadBuffer)
    {
        return m_UploadBuffer;
    }

    CCryDX12Buffer* resource = nullptr;
    GetDevice()->CreateStagingResource(this, (ID3D11Resource**)&resource, true);
    m_UploadBuffer.attach(resource);
    return m_UploadBuffer.get();
}

void CCryDX12Buffer::MapDiscard(DX12::CommandList* pCmdList)
{
    Super::MapDiscard(pCmdList);
    m_DX12View.Init(m_DX12Resource, DX12::EVT_ConstantBufferView, m_Desc11.ByteWidth);
}

#pragma region /* ID3D11Buffer implementation */

void STDMETHODCALLTYPE CCryDX12Buffer::GetDesc(
    _Out_  D3D11_BUFFER_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = m_Desc11;
    }
}

#pragma endregion
