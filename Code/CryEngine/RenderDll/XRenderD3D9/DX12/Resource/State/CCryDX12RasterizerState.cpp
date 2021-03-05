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
#include "CCryDX12RasterizerState.hpp"

CCryDX12RasterizerState* CCryDX12RasterizerState::Create(const D3D11_RASTERIZER_DESC* pRasterizerDesc)
{
    D3D12_RASTERIZER_DESC desc12;
    ZeroMemory(&desc12, sizeof(D3D11_RASTERIZER_DESC));

    desc12.AntialiasedLineEnable = pRasterizerDesc->AntialiasedLineEnable;
    desc12.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    desc12.CullMode = static_cast<D3D12_CULL_MODE>(pRasterizerDesc->CullMode);
    desc12.DepthBias = pRasterizerDesc->DepthBias;
    desc12.DepthBiasClamp = pRasterizerDesc->DepthBiasClamp;
    desc12.DepthClipEnable = pRasterizerDesc->DepthClipEnable;
    desc12.FillMode = static_cast<D3D12_FILL_MODE>(pRasterizerDesc->FillMode);
    desc12.ForcedSampleCount = 0;
    desc12.FrontCounterClockwise = pRasterizerDesc->FrontCounterClockwise;
    desc12.MultisampleEnable = pRasterizerDesc->MultisampleEnable;
    desc12.SlopeScaledDepthBias = pRasterizerDesc->SlopeScaledDepthBias;

    return DX12::PassAddRef(new CCryDX12RasterizerState(*pRasterizerDesc, desc12));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12RasterizerState::CCryDX12RasterizerState(const D3D11_RASTERIZER_DESC& desc11, const D3D12_RASTERIZER_DESC& desc12)
    : Super(nullptr, nullptr)
    , m_Desc11(desc11)
    , m_Desc12(desc12)
{
}

CCryDX12RasterizerState::~CCryDX12RasterizerState()
{
}

#pragma region /* ID3D11RasterizerState implementation */

void STDMETHODCALLTYPE CCryDX12RasterizerState::GetDesc(
    _Out_  D3D11_RASTERIZER_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = m_Desc11;
    }
}

#pragma endregion
