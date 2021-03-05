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
#include "CCryDX12BlendState.hpp"

CCryDX12BlendState* CCryDX12BlendState::Create(const D3D11_BLEND_DESC* pBlendStateDesc)
{
    D3D12_BLEND_DESC desc12;
    ZeroMemory(&desc12, sizeof(D3D11_BLEND_DESC));

    desc12.AlphaToCoverageEnable = pBlendStateDesc->AlphaToCoverageEnable;
    desc12.IndependentBlendEnable = pBlendStateDesc->IndependentBlendEnable;

    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        const D3D11_RENDER_TARGET_BLEND_DESC& rt11 = pBlendStateDesc->RenderTarget[i];
        D3D12_RENDER_TARGET_BLEND_DESC& rt12 = desc12.RenderTarget[i];

        rt12.BlendEnable = rt11.BlendEnable;
        rt12.BlendOp = static_cast<D3D12_BLEND_OP>(rt11.BlendOp);
        rt12.BlendOpAlpha = static_cast<D3D12_BLEND_OP>(rt11.BlendOpAlpha);
        rt12.DestBlend = static_cast<D3D12_BLEND>(rt11.DestBlend);
        rt12.DestBlendAlpha = static_cast<D3D12_BLEND>(rt11.DestBlendAlpha);
        rt12.SrcBlend = static_cast<D3D12_BLEND>(rt11.SrcBlend);
        rt12.SrcBlendAlpha = static_cast<D3D12_BLEND>(rt11.SrcBlendAlpha);
        rt12.RenderTargetWriteMask = rt11.RenderTargetWriteMask;
        rt12.LogicOpEnable = FALSE;
        rt12.LogicOp = D3D12_LOGIC_OP_CLEAR;
    }

    return DX12::PassAddRef(new CCryDX12BlendState(*pBlendStateDesc, desc12));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12BlendState::CCryDX12BlendState(const D3D11_BLEND_DESC& desc11, const D3D12_BLEND_DESC& desc12)
    : Super(nullptr, nullptr)
    , m_Desc11(desc11)
    , m_Desc12(desc12)
{
}

CCryDX12BlendState::~CCryDX12BlendState()
{
}

#pragma region /* ID3D11BlendState implementation */

void STDMETHODCALLTYPE CCryDX12BlendState::GetDesc(
    _Out_  D3D11_BLEND_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = m_Desc11;
    }
}

#pragma endregion
