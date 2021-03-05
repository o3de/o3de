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

// Description : Definition of the DXGL wrapper for ID3D11Query


#include "RenderDll_precompiled.h"
#include "CCryDXGLQuery.hpp"
#include "../Implementation/GLResource.hpp"


CCryDXGLQuery::CCryDXGLQuery(const D3D11_QUERY_DESC& kDesc, NCryOpenGL::SQuery* pGLQuery, CCryDXGLDevice* pDevice)
    : CCryDXGLDeviceChild(pDevice)
    , m_kDesc(kDesc)
    , m_spGLQuery(pGLQuery)
{
    DXGL_INITIALIZE_INTERFACE(D3D11Asynchronous)
    DXGL_INITIALIZE_INTERFACE(D3D11Query)
}

CCryDXGLQuery::~CCryDXGLQuery()
{
}

NCryOpenGL::SQuery* CCryDXGLQuery::GetGLQuery()
{
    return m_spGLQuery;
}


////////////////////////////////////////////////////////////////////////////////
// ID3D11Asynchronous implementation
////////////////////////////////////////////////////////////////////////////////

UINT CCryDXGLQuery::GetDataSize(void)
{
    return m_spGLQuery->GetDataSize();
}


////////////////////////////////////////////////////////////////////////////////
// ID3D11Query implementation
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLQuery::GetDesc(D3D11_QUERY_DESC* pDesc)
{
    (*pDesc) = m_kDesc;
}