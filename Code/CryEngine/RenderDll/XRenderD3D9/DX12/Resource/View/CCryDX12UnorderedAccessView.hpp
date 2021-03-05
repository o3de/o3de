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
#ifndef __CCRYDX12UNORDEREDACCESSVIEW__
#define __CCRYDX12UNORDEREDACCESSVIEW__

#include "DX12/Resource/CCryDX12View.hpp"

class CCryDX12UnorderedAccessView
    : public CCryDX12View<ID3D11UnorderedAccessView>
{
public:
    DX12_OBJECT(CCryDX12UnorderedAccessView, CCryDX12View<ID3D11UnorderedAccessView>);

    static CCryDX12UnorderedAccessView* Create(CCryDX12Device* pDevice, ID3D11Resource* pResource11, const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc11);

    virtual ~CCryDX12UnorderedAccessView();

    #pragma region /* ID3D11UnorderedAccessView implementation */

    virtual void STDMETHODCALLTYPE GetDesc(
        _Out_  D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc) final;

    #pragma endregion

    template<class T>
    void BeginResourceStateTransition([[maybe_unused]] T* pCmdList)
    {
        //pCmdList->QueueTransitionBarrierBegin(GetDX12View().GetDX12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    template<class T>
    void EndResourceStateTransition(T* pCmdList)
    {
        pCmdList->QueueTransitionBarrier(GetDX12View().GetDX12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

protected:
    CCryDX12UnorderedAccessView(ID3D11Resource* pResource11, const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc11, ID3D12Resource* pResource12, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc12);

private:
    D3D11_UNORDERED_ACCESS_VIEW_DESC m_Desc11;
};

#endif // __CCRYDX12UNORDEREDACCESSVIEW__
