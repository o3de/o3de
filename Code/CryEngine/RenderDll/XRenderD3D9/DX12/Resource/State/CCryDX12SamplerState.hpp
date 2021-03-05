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
#ifndef __CCRYDX12SAMPLERSTATE__
#define __CCRYDX12SAMPLERSTATE__

#include "DX12/Device/CCryDX12DeviceChild.hpp"

#include "DX12/API/DX12SamplerState.hpp"

class CCryDX12SamplerState
    : public CCryDX12DeviceChild<ID3D11SamplerState>
{
public:
    DX12_OBJECT(CCryDX12SamplerState, CCryDX12DeviceChild<ID3D11SamplerState>);

    static CCryDX12SamplerState* Create(const D3D11_SAMPLER_DESC* pSamplerDesc);

    DX12::SamplerState& GetDX12SamplerState()
    {
        return m_DX12SamplerState;
    }

    #pragma region /* ID3D11SamplerState implementation */

    virtual void STDMETHODCALLTYPE GetDesc(
        _Out_  D3D11_SAMPLER_DESC* pDesc);

    #pragma endregion

protected:
    CCryDX12SamplerState(const D3D11_SAMPLER_DESC& desc11, const D3D12_SAMPLER_DESC& desc12)
        : Super(nullptr, nullptr)
        , m_Desc11(desc11)
    {
        m_DX12SamplerState.GetSamplerDesc() = desc12;
    }

private:
    D3D11_SAMPLER_DESC m_Desc11;

    DX12::SamplerState m_DX12SamplerState;
};

#endif // __CCRYDX12SAMPLERSTATE__
