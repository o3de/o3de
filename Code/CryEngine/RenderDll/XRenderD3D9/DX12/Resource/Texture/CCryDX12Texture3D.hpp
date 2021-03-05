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
#ifndef __CCRYDX12TEXTURE3D__
#define __CCRYDX12TEXTURE3D__

#include "DX12/Resource/CCryDX12Resource.hpp"

class CCryDX12Texture3D
    : public CCryDX12Resource<ID3D11Texture3D>
{
public:
    DX12_OBJECT(CCryDX12Texture3D, CCryDX12Resource<ID3D11Texture3D>);

    static CCryDX12Texture3D* Create(CCryDX12Device* pDevice, CCryDX12SwapChain* pSwapChain, ID3D12Resource* pResource);

    static CCryDX12Texture3D* Create(CCryDX12Device* pDevice, const FLOAT cClearValue[4], const D3D11_TEXTURE3D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData);

    virtual ~CCryDX12Texture3D();

    #pragma region /* ICryDX12Resource implementation */

    virtual EDX12ResourceType GetDX12ResourceType() const final
    {
        return eDX12RT_Texture3D;
    }

    virtual void STDMETHODCALLTYPE GetType(_Out_  D3D11_RESOURCE_DIMENSION* pResourceDimension) final
    {
        if (pResourceDimension)
        {
            *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
        }
    }

    #pragma endregion

    #pragma region /* ID3D11Texture3D implementation */

    virtual void STDMETHODCALLTYPE GetDesc(
        _Out_  D3D11_TEXTURE3D_DESC* pDesc) final
    {
        if (pDesc)
        {
            *pDesc = m_Desc11;
        }
    }

    #pragma endregion

protected:
    CCryDX12Texture3D(CCryDX12Device* pDevice, const D3D11_TEXTURE3D_DESC& desc11, ID3D12Resource* pResource, D3D12_RESOURCE_STATES eInitialState, const CD3DX12_RESOURCE_DESC& desc12, const D3D11_SUBRESOURCE_DATA* pInitialData = NULL, size_t numInitialData = 0);

private:
    D3D11_TEXTURE3D_DESC m_Desc11;
};

#endif // __CCRYDX12TEXTURE3D__
