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
#ifndef __CCRYDX12DEPTHSTENCILSTATE__
#define __CCRYDX12DEPTHSTENCILSTATE__

#include "DX12/Device/CCryDX12DeviceChild.hpp"

class CCryDX12DepthStencilState
    : public CCryDX12DeviceChild<ID3D11DepthStencilState>
{
public:
    DX12_OBJECT(CCryDX12DepthStencilState, CCryDX12DeviceChild<ID3D11DepthStencilState>);

    static CCryDX12DepthStencilState* Create(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc);

    virtual ~CCryDX12DepthStencilState();

    const D3D12_DEPTH_STENCIL_DESC& GetD3D12DepthStencilDesc() const
    {
        return m_Desc12;
    }

    #pragma region /* ID3D11DepthStencilState implementation */

    virtual void STDMETHODCALLTYPE GetDesc(
        _Out_  D3D11_DEPTH_STENCIL_DESC* pDesc);

    #pragma endregion

protected:
    CCryDX12DepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc11, const D3D12_DEPTH_STENCIL_DESC& desc12);

private:
    D3D11_DEPTH_STENCIL_DESC m_Desc11;
    D3D12_DEPTH_STENCIL_DESC m_Desc12;
};

#endif // __CCRYDX12DEPTHSTENCILSTATE__
