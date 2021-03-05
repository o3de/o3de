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
#include "CCryDX12Texture1D.hpp"
#include "../GI/CCryDX12SwapChain.hpp"

#include "DX12/Device/CCryDX12Device.hpp"

CCryDX12Texture1D* CCryDX12Texture1D::Create(CCryDX12Device* pDevice, CCryDX12SwapChain* pSwapChain, ID3D12Resource* pResource)
{
    DX12_ASSERT(pResource);

    D3D12_RESOURCE_DESC desc12 = pResource->GetDesc();

    D3D11_TEXTURE1D_DESC desc11;
    desc11.Width = static_cast<UINT>(desc12.Width);
    desc11.MipLevels = static_cast<UINT>(desc12.MipLevels);
    desc11.ArraySize = static_cast<UINT>(desc12.DepthOrArraySize);
    desc11.Format = desc12.Format;
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

    CCryDX12Texture1D* result = DX12::PassAddRef(new CCryDX12Texture1D(pDevice, desc11, pResource, D3D12_RESOURCE_STATE_RENDER_TARGET, CD3DX12_RESOURCE_DESC(desc12), NULL));
    result->GetDX12Resource().SetDX12SwapChain(pSwapChain->GetDX12SwapChain());

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Texture1D* CCryDX12Texture1D::Create(CCryDX12Device* pDevice, const FLOAT cClearValue[4], const D3D11_TEXTURE1D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData)
{
    DX12_ASSERT(pDesc, "CreateTexture() called without description!");

    CD3DX12_RESOURCE_DESC desc12 = CD3DX12_RESOURCE_DESC::Tex1D(
            pDesc->Format,
            pDesc->Width,
            pDesc->ArraySize,
            pDesc->MipLevels,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_TEXTURE_LAYOUT_UNKNOWN,

            // Alignment
            0);

    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_STATES resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_CLEAR_VALUE clearValue = DX12::GetDXGIFormatClearValue(desc12.Format, (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0);
    bool allowClearValue = desc12.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
    {
        resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    else if (pDesc->Usage == D3D11_USAGE_STAGING)
    {
        resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    else if (pDesc->Usage == D3D11_USAGE_DYNAMIC)
    {
        resourceUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    if (pDesc->CPUAccessFlags != 0)
    {
        if (pDesc->CPUAccessFlags == D3D11_CPU_ACCESS_WRITE)
        {
            resourceUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else if (pDesc->CPUAccessFlags == D3D11_CPU_ACCESS_READ)
        {
            resourceUsage = D3D12_RESOURCE_STATE_COPY_DEST;
        }
        else
        {
            DX12_NOT_IMPLEMENTED;
            return nullptr;
        }
    }

    if (cClearValue)
    {
        if (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
        {
            clearValue.DepthStencil.Depth   = FLOAT(cClearValue[0]);
            clearValue.DepthStencil.Stencil = UINT8(cClearValue[1]);
        }
        else
        {
            clearValue.Color[0] = cClearValue[0];
            clearValue.Color[1] = cClearValue[1];
            clearValue.Color[2] = cClearValue[2];
            clearValue.Color[3] = cClearValue[3];
        }
    }

    if (pDesc->BindFlags & D3D11_BIND_UNORDERED_ACCESS)
    {
        desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        allowClearValue = true;
        resourceUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    if (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        allowClearValue = true;
        resourceUsage = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }

    if (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)
    {
        desc12.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        allowClearValue = true;
        resourceUsage = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }

    ID3D12Resource* resource = NULL;

    if (S_OK != pDevice->GetD3D12Device()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc12,
            resourceUsage,
            allowClearValue ? &clearValue : NULL,
            IID_GRAPHICS_PPV_ARGS(&resource)
            ) || !resource)
    {
        DX12_ASSERT(0, "Could not create texture 1D resource!");
        return NULL;
    }

    auto result = DX12::PassAddRef(new CCryDX12Texture1D(pDevice, *pDesc, resource, resourceUsage, CD3DX12_RESOURCE_DESC(resource->GetDesc()), pInitialData, desc12.DepthOrArraySize * desc12.MipLevels));
    resource->Release();

#if !defined(RELEASE) && defined(GFX_DEBUG)
    resource->SetPrivateData(WKPDID_D3DDebugClearValue, sizeof(clearValue), &clearValue);
#endif

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Texture1D::CCryDX12Texture1D(CCryDX12Device* pDevice, const D3D11_TEXTURE1D_DESC& desc11, ID3D12Resource* pResource, D3D12_RESOURCE_STATES eInitialState, const CD3DX12_RESOURCE_DESC& desc12, const D3D11_SUBRESOURCE_DATA* pInitialData, size_t numInitialData)
    : Super(pDevice, pResource, eInitialState, desc12, pInitialData, numInitialData)
    , m_Desc11(desc11)
{
}

CCryDX12Texture1D::~CCryDX12Texture1D()
{
}
