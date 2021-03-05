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
#include "CCryDX12Query.hpp"

CCryDX12Query* CCryDX12Query::Create(ID3D12Device* pDevice, const D3D11_QUERY_DESC* pDesc)
{
    switch (pDesc->Query)
    {
    case D3D11_QUERY_EVENT:
    {
        auto pResult = DX12::PassAddRef(new CCryDX12EventQuery(pDesc));
        if (!pResult->Init(pDevice))
        {
            SAFE_RELEASE(pResult);
        }

        return pResult;
    }
    break;
    case D3D11_QUERY_TIMESTAMP:
    case D3D11_QUERY_OCCLUSION:
    case D3D11_QUERY_OCCLUSION_PREDICATE:
    {
        auto pResult = DX12::PassAddRef(new CCryDX12ResourceQuery(pDesc));
        if (!pResult->Init(pDevice))
        {
            SAFE_RELEASE(pResult);
        }

        return pResult;
    }
    break;

    case D3D11_QUERY_TIMESTAMP_DISJOINT:
    default:
        return DX12::PassAddRef(new CCryDX12Query(pDesc));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Query::CCryDX12Query(const D3D11_QUERY_DESC* pDesc)
    : Super()
    , m_Desc(*pDesc)
{
}

CCryDX12Query::~CCryDX12Query()
{
}

/* ID3D11Query implementation */

void STDMETHODCALLTYPE CCryDX12Query::GetDesc(
    _Out_  D3D11_QUERY_DESC* pDesc)
{
    *pDesc = m_Desc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12EventQuery::CCryDX12EventQuery(const D3D11_QUERY_DESC* pDesc)
    : Super(pDesc)
    , m_FenceValue(0)
{
}

CCryDX12EventQuery::~CCryDX12EventQuery()
{
}

bool CCryDX12EventQuery::Init([[maybe_unused]] ID3D12Device* pDevice)
{
    m_FenceValue = 0LL;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12ResourceQuery::CCryDX12ResourceQuery(const D3D11_QUERY_DESC* pDesc)
    : Super(pDesc)
    , m_QueryIndex(0)
    , m_pResource(nullptr)
{
}

CCryDX12ResourceQuery::~CCryDX12ResourceQuery()
{
}

bool CCryDX12ResourceQuery::Init(ID3D12Device* pDevice)
{
    Super::Init(pDevice);
    m_QueryIndex = 0;
    m_pResource = nullptr;
    return true;
}
