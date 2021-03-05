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
#include "CCryDX12ShaderResourceView.hpp"

#include "DX12/Device/CCryDX12Device.hpp"
#include "DX12/Resource/Misc/CCryDX12Buffer.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture2D.hpp"

static UINT GetPlaneSlice(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return 1;
    default:
        return 0;
    }
}

CCryDX12ShaderResourceView* CCryDX12ShaderResourceView::Create([[maybe_unused]] CCryDX12Device* pDevice, ID3D11Resource* pResource11, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
    ID3D12Resource* pD3D12Resource = DX12_EXTRACT_D3D12RESOURCE(pResource11);
    if (!pD3D12Resource)
    {
        DX12_ASSERT(0, "Unknown resource type!");
        return NULL;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC desc12;
    ZeroMemory(&desc12, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

    desc12.Format = pDesc->Format;
    desc12.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc12.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(pDesc->ViewDimension);
    switch (desc12.ViewDimension)
    {
    case D3D12_SRV_DIMENSION_BUFFER:
        desc12.Buffer.FirstElement                  = pDesc->Buffer.FirstElement;
        desc12.Buffer.NumElements                   = pDesc->Buffer.NumElements;
        desc12.Buffer.Flags                         = D3D12_BUFFER_SRV_FLAG_NONE;
        desc12.Buffer.StructureByteStride           = DX12_EXTRACT_BUFFER(pResource11)->GetStructureByteStride();
        break;
    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
        desc12.Texture1DArray.ArraySize             = pDesc->Texture1DArray.ArraySize;
        desc12.Texture1DArray.FirstArraySlice       = pDesc->Texture1DArray.FirstArraySlice;
        desc12.Texture1DArray.MipLevels             = pDesc->Texture1DArray.MipLevels;
        desc12.Texture1DArray.MostDetailedMip       = pDesc->Texture1DArray.MostDetailedMip;
        desc12.Texture1DArray.ResourceMinLODClamp   = 0.0f;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
        desc12.Texture2DArray.ArraySize             = pDesc->Texture2DArray.ArraySize;
        desc12.Texture2DArray.FirstArraySlice       = pDesc->Texture2DArray.FirstArraySlice;
        desc12.Texture2DArray.MipLevels             = pDesc->Texture2DArray.MipLevels;
        desc12.Texture2DArray.MostDetailedMip       = pDesc->Texture2DArray.MostDetailedMip;
        desc12.Texture2DArray.ResourceMinLODClamp   = 0.0f;
        desc12.Texture2DArray.PlaneSlice            = GetPlaneSlice(pDesc->Format);
        break;
    case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
        desc12.TextureCubeArray.First2DArrayFace    = pDesc->TextureCubeArray.First2DArrayFace;
        desc12.TextureCubeArray.MipLevels           = pDesc->TextureCubeArray.MipLevels;
        desc12.TextureCubeArray.MostDetailedMip     = pDesc->TextureCubeArray.MostDetailedMip;
        desc12.TextureCubeArray.NumCubes            = pDesc->TextureCubeArray.NumCubes;
        desc12.TextureCubeArray.ResourceMinLODClamp = 0.0f;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2DMS:
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
        desc12.Texture2DMSArray.FirstArraySlice     = pDesc->Texture2DMSArray.FirstArraySlice;
        desc12.Texture2DMSArray.ArraySize           = pDesc->Texture2DMSArray.ArraySize;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE1D:
        desc12.Texture1D.MipLevels                  = pDesc->Texture1D.MipLevels;
        desc12.Texture1D.MostDetailedMip            = pDesc->Texture1D.MostDetailedMip;
        desc12.Texture1D.ResourceMinLODClamp        = 0.0f;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2D:
        desc12.Texture2D.MipLevels                  = pDesc->Texture2D.MipLevels;
        desc12.Texture2D.MostDetailedMip            = pDesc->Texture2D.MostDetailedMip;
        desc12.Texture2D.ResourceMinLODClamp        = 0.0f;
        desc12.Texture2D.PlaneSlice                 = GetPlaneSlice(pDesc->Format);
        break;
    case D3D12_SRV_DIMENSION_TEXTURECUBE:
        desc12.TextureCube.MipLevels                = pDesc->TextureCube.MipLevels;
        desc12.TextureCube.MostDetailedMip          = pDesc->TextureCube.MostDetailedMip;
        desc12.TextureCube.ResourceMinLODClamp      = 0.0f;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE3D:
        desc12.Texture3D.MipLevels                  = pDesc->Texture3D.MipLevels;
        desc12.Texture3D.MostDetailedMip            = pDesc->Texture3D.MostDetailedMip;
        desc12.Texture3D.ResourceMinLODClamp        = 0.0f;
        break;
    }

    return DX12::PassAddRef(new CCryDX12ShaderResourceView(pResource11, *pDesc, pD3D12Resource, desc12));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12ShaderResourceView::CCryDX12ShaderResourceView(ID3D11Resource* pResource11, const D3D11_SHADER_RESOURCE_VIEW_DESC& desc11, [[maybe_unused]] ID3D12Resource* pResource12, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc12)
    : Super(pResource11, DX12::EVT_ShaderResourceView)
    , m_Desc11(desc11)
{
    m_DX12View.GetSRVDesc() = desc12;
}

CCryDX12ShaderResourceView::~CCryDX12ShaderResourceView()
{
}

/* ID3D11ShaderResourceView implementation */

void STDMETHODCALLTYPE CCryDX12ShaderResourceView::GetDesc(
    _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = m_Desc11;
    }
}
