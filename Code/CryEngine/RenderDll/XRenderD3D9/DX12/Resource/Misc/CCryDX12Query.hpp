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
#ifndef __CCRYDX12QUERY__
#define __CCRYDX12QUERY__

#include "DX12/Resource/CCryDX12Asynchronous.hpp"

class CCryDX12Query
    : public CCryDX12Asynchronous<ID3D11Query>
{
public:
    DX12_OBJECT(CCryDX12Query, CCryDX12Asynchronous<ID3D11Query>);

    static CCryDX12Query* Create(ID3D12Device* pDevice, const D3D11_QUERY_DESC* pDesc);

    virtual ~CCryDX12Query();

    #pragma region /* ID3D11Asynchronous implementation */

    virtual UINT STDMETHODCALLTYPE GetDataSize()
    {
        if (m_Desc.Query == D3D11_QUERY_EVENT)
        {
            return sizeof(BOOL);
        }
        else if (m_Desc.Query == D3D11_QUERY_TIMESTAMP)
        {
            return sizeof(UINT64);
        }
        else if (m_Desc.Query == D3D11_QUERY_TIMESTAMP_DISJOINT)
        {
            return sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT);
        }
        else if (m_Desc.Query == D3D11_QUERY_OCCLUSION)
        {
            return sizeof(UINT64);
        }
        else if (m_Desc.Query == D3D11_QUERY_OCCLUSION_PREDICATE)
        {
            return sizeof(UINT64);
        }
        else if (m_Desc.Query == D3D11_QUERY_PIPELINE_STATISTICS)
        {
            return sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS);
        }
        else
        {
            return 0;
        }
    }

    #pragma endregion

    #pragma region /* ID3D11Query implementation */

    virtual void STDMETHODCALLTYPE GetDesc(
        _Out_  D3D11_QUERY_DESC* pDesc);

    #pragma endregion

protected:
    CCryDX12Query(const D3D11_QUERY_DESC* pDesc);

    D3D11_QUERY_DESC m_Desc;
};

class CCryDX12EventQuery
    : public CCryDX12Query
{
public:
    DX12_OBJECT(CCryDX12EventQuery, CCryDX12Query);

    CCryDX12EventQuery(const D3D11_QUERY_DESC* pDesc);
    virtual ~CCryDX12EventQuery();

    bool Init(ID3D12Device* pDevice);

    UINT64 GetFenceValue() const
    {
        return m_FenceValue;
    }

    void SetFenceValue(UINT64 fenceValue)
    {
        m_FenceValue = fenceValue;
    }

private:
    UINT64 m_FenceValue;
};

class CCryDX12ResourceQuery
    : public CCryDX12EventQuery
{
public:
    DX12_OBJECT(CCryDX12ResourceQuery, CCryDX12EventQuery);

    CCryDX12ResourceQuery(const D3D11_QUERY_DESC* pDesc);
    virtual ~CCryDX12ResourceQuery();

    bool Init(ID3D12Device* pDevice);

    UINT GetQueryIndex() const
    {
        return m_QueryIndex;
    }

    void SetQueryIndex(UINT queryIndex)
    {
        m_QueryIndex = queryIndex;
    }

    ID3D12Resource* GetQueryResource() const
    {
        return m_pResource;
    }

    void SetQueryResource(ID3D12Resource* pResource)
    {
        m_pResource = pResource;
    }

private:
    UINT m_QueryIndex;
    ID3D12Resource* m_pResource;
};


#endif // __CCRYDX12QUERY__
