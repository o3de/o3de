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
#include "CCryDX12DepthStencilState.hpp"

CCryDX12DepthStencilState* CCryDX12DepthStencilState::Create(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc)
{
    D3D12_DEPTH_STENCIL_DESC desc12;
    ZeroMemory(&desc12, sizeof(D3D12_DEPTH_STENCIL_DESC));

    desc12.BackFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(pDepthStencilDesc->BackFace.StencilDepthFailOp);
    desc12.BackFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(pDepthStencilDesc->BackFace.StencilFailOp);
    desc12.BackFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(pDepthStencilDesc->BackFace.StencilPassOp);
    desc12.BackFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(pDepthStencilDesc->BackFace.StencilFunc);

    desc12.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(pDepthStencilDesc->FrontFace.StencilDepthFailOp);
    desc12.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(pDepthStencilDesc->FrontFace.StencilFailOp);
    desc12.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(pDepthStencilDesc->FrontFace.StencilPassOp);
    desc12.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(pDepthStencilDesc->FrontFace.StencilFunc);

    desc12.DepthEnable = pDepthStencilDesc->DepthEnable;
    desc12.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(pDepthStencilDesc->DepthFunc);
    desc12.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(pDepthStencilDesc->DepthWriteMask);
    desc12.StencilEnable = pDepthStencilDesc->StencilEnable;
    desc12.StencilReadMask = pDepthStencilDesc->StencilReadMask;
    desc12.StencilWriteMask = pDepthStencilDesc->StencilWriteMask;

    return DX12::PassAddRef(new CCryDX12DepthStencilState(*pDepthStencilDesc, desc12));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12DepthStencilState::CCryDX12DepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc11, const D3D12_DEPTH_STENCIL_DESC& desc12)
    : Super(nullptr, nullptr)
    , m_Desc11(desc11)
    , m_Desc12(desc12)
{
}

CCryDX12DepthStencilState::~CCryDX12DepthStencilState()
{
}

#pragma region /* ID3D11DepthStencilState implementation */

void STDMETHODCALLTYPE CCryDX12DepthStencilState::GetDesc(
    _Out_  D3D11_DEPTH_STENCIL_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = m_Desc11;
    }
}

#pragma endregion
