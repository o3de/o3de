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
#include "CCryDX12UnorderedAccessView.hpp"

CCryDX12UnorderedAccessView* CCryDX12UnorderedAccessView::Create([[maybe_unused]] CCryDX12Device* pDevice, ID3D11Resource* pResource11, const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc11)
{
    ID3D12Resource* pD3D12Resource = DX12_EXTRACT_D3D12RESOURCE(pResource11);
    if (!pD3D12Resource)
    {
        DX12_ASSERT(0, "Unknown resource type!");
        return NULL;
    }

    DX12_ASSERT(pDesc11->ViewDimension != D3D11_UAV_DIMENSION_BUFFER || !(pDesc11->Buffer.Flags & D3D11_BUFFER_UAV_FLAG_APPEND), "No append/consume supported under DX12!");
    DX12_ASSERT(pDesc11->ViewDimension != D3D11_UAV_DIMENSION_BUFFER || !(pDesc11->Buffer.Flags & D3D11_BUFFER_UAV_FLAG_COUNTER), "Counters have significantly changed under DX12! Please rewrite the algorithm for DX12 on a higher level.");

    D3D12_UNORDERED_ACCESS_VIEW_DESC desc12;
    ZeroMemory(&desc12, sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC));

    desc12.Format = pDesc11->Format;
    desc12.ViewDimension = static_cast<D3D12_UAV_DIMENSION>(pDesc11->ViewDimension);
    switch (desc12.ViewDimension)
    {
    case D3D12_UAV_DIMENSION_BUFFER:
        desc12.Buffer.FirstElement            = static_cast<UINT64>(pDesc11->Buffer.FirstElement);
        desc12.Buffer.NumElements             = pDesc11->Buffer.NumElements;
        desc12.Buffer.Flags                   = (pDesc11->Buffer.Flags & D3D11_BUFFER_UAV_FLAG_RAW ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE);
        desc12.Buffer.CounterOffsetInBytes    = 0;     // NOTE: not yet implemented/supported
        desc12.Buffer.StructureByteStride     = DX12_EXTRACT_BUFFER(pResource11)->GetStructureByteStride();
        break;
    case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
        desc12.Texture1DArray.MipSlice        = pDesc11->Texture1DArray.MipSlice;
        desc12.Texture1DArray.FirstArraySlice = pDesc11->Texture1DArray.FirstArraySlice;
        desc12.Texture1DArray.ArraySize       = pDesc11->Texture1DArray.ArraySize;
        break;
    case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
        desc12.Texture2DArray.MipSlice        = pDesc11->Texture2DArray.MipSlice;
        desc12.Texture2DArray.FirstArraySlice = pDesc11->Texture2DArray.FirstArraySlice;
        desc12.Texture2DArray.ArraySize       = pDesc11->Texture2DArray.ArraySize;
        desc12.Texture2DArray.PlaneSlice      = 0;     // NOTE: not yet implemented/supported
        break;
    case D3D12_UAV_DIMENSION_TEXTURE1D:
        desc12.Texture1D.MipSlice             = pDesc11->Texture1D.MipSlice;
        break;
    case D3D12_UAV_DIMENSION_TEXTURE2D:
        desc12.Texture2D.MipSlice             = pDesc11->Texture2D.MipSlice;
        desc12.Texture2D.PlaneSlice           = 0;     // NOTE: not yet implemented/supported
        break;
    case D3D12_UAV_DIMENSION_TEXTURE3D:
        desc12.Texture3D.MipSlice             = pDesc11->Texture3D.MipSlice;
        desc12.Texture3D.FirstWSlice          = pDesc11->Texture3D.FirstWSlice;
        desc12.Texture3D.WSize                = pDesc11->Texture3D.WSize;
        break;
    }

    return DX12::PassAddRef(new CCryDX12UnorderedAccessView(pResource11, *pDesc11, pD3D12Resource, desc12));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12UnorderedAccessView::CCryDX12UnorderedAccessView(ID3D11Resource* pResource11, const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc11, [[maybe_unused]] ID3D12Resource* pResource12, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc12)
    : Super(pResource11, DX12::EVT_UnorderedAccessView)
    , m_Desc11(desc11)
{
    m_DX12View.GetUAVDesc() = desc12;
}

CCryDX12UnorderedAccessView::~CCryDX12UnorderedAccessView()
{
}

#pragma region /* ID3D11UnorderedAccessView implementation */

void STDMETHODCALLTYPE CCryDX12UnorderedAccessView::GetDesc(
    _Out_  D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = m_Desc11;
    }
}

#pragma endregion
