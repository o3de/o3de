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

// Description : Occlusion queries unified interface implementation


#include "RenderDll_precompiled.h"
#include "DriverD3D.h"

void COcclusionQuery::Create()
{
    Release();

    // Create visibility queries

    D3DQuery* pVizQuery = NULL;
    D3D11_QUERY_DESC desc;
    desc.Query = D3D11_QUERY_OCCLUSION;
    desc.MiscFlags = 0;
    HRESULT hr(gcpRendD3D->GetDevice().CreateQuery(&desc, &pVizQuery));
    assert(SUCCEEDED(hr));

    m_nOcclusionID = (UINT_PTR) pVizQuery;
    
    // Prime query
    BeginQuery();
    EndQuery();
}

void COcclusionQuery::Release()
{
    D3DQuery* pVizQuery = (D3DQuery*)m_nOcclusionID;
    SAFE_RELEASE(pVizQuery);

    m_nOcclusionID = 0;
    m_nDrawFrame = 0;
    m_nCheckFrame = 0;
    m_nVisSamples = ~0;
}

void COcclusionQuery::BeginQuery()
{
    if (!m_nOcclusionID)
    {
        return;
    }

    D3DQuery* pVizQuery = (D3DQuery*)m_nOcclusionID;
    gcpRendD3D->GetDeviceContext().Begin(pVizQuery);
}

void COcclusionQuery::EndQuery()
{
    if (!m_nOcclusionID)
    {
        return;
    }

    CD3D9Renderer* rd = gcpRendD3D;
    m_nDrawFrame = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

    D3DQuery*  pVizQuery = (D3DQuery*)m_nOcclusionID;
    rd->GetDeviceContext().End(pVizQuery);
}

bool COcclusionQuery::IsReady()
{
    CD3D9Renderer* rd = gcpRendD3D;
    int nFrame = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    return (m_nCheckFrame == nFrame);
}

uint32 COcclusionQuery::GetVisibleSamples(bool bAsynchronous)
{
    if (!m_nOcclusionID)
    {
        return ~0;
    }

    CD3D9Renderer* rd = gcpRendD3D;
    int nFrame = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID;

    if (m_nCheckFrame == nFrame)
    {
        return m_nVisSamples;
    }

    int64 nVisSamples = ~0; //query needs to be 8byte

    if (!bAsynchronous)
    {
        PROFILE_FRAME(COcclusionQuery::GetVisibleSamples);

        HRESULT hRes = S_FALSE;

        D3DQuery* pVizQuery = (D3DQuery*)m_nOcclusionID;

        size_t DataSize =   pVizQuery->GetDataSize();
        assert(sizeof(nVisSamples) == DataSize);
        while (hRes == S_FALSE)
        {
            hRes = rd->GetDeviceContext().GetData(pVizQuery, (void*) &nVisSamples, DataSize, 0);
        }

        if (hRes == S_OK)
        {
            m_nCheckFrame = nFrame;
            m_nVisSamples = static_cast<int>(nVisSamples);
        }
    }
    else
    {
        PROFILE_FRAME(COcclusionQuery::GetVisibleSamplesAsync);

        HRESULT hRes = S_OK;

        D3DQuery*  pVizQuery = (D3DQuery*)m_nOcclusionID;
        hRes = rd->GetDeviceContext().GetData(pVizQuery, (void*) &nVisSamples, pVizQuery->GetDataSize(), 0);

        if (hRes == S_OK)
        {
            m_nCheckFrame = nFrame;
            m_nVisSamples = static_cast<int>(nVisSamples);
        }
    }
    return m_nVisSamples;
}
