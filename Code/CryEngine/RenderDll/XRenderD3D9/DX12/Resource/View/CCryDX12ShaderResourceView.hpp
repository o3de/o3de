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
#ifndef __CRYDX12SHADERRESOURCEVIEW__
#define __CRYDX12SHADERRESOURCEVIEW__

#include "DX12/Resource/CCryDX12View.hpp"

class CCryDX12ShaderResourceView
    : public CCryDX12View<ID3D11ShaderResourceView>
{
public:
    DX12_OBJECT(CCryDX12ShaderResourceView, CCryDX12View<ID3D11ShaderResourceView>);

    static CCryDX12ShaderResourceView* Create(CCryDX12Device* pDevice, ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc);

    virtual ~CCryDX12ShaderResourceView();

    #pragma region /* ID3D11ShaderResourceView implementation */

    virtual void STDMETHODCALLTYPE GetDesc(
        _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc) final;

    #pragma endregion

    void BeginResourceStateTransition([[maybe_unused]] DX12::CommandList* commandList)
    {
        //commandList->QueueTransitionBarrierBegin(GetDX12View().GetDX12Resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

protected:
    CCryDX12ShaderResourceView(ID3D11Resource* pResource11, const D3D11_SHADER_RESOURCE_VIEW_DESC& desc11, ID3D12Resource* pResource12, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc12);

private:
    D3D11_SHADER_RESOURCE_VIEW_DESC m_Desc11;
};

#endif
