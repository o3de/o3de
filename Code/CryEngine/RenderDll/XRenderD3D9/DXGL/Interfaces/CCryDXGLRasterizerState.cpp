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

// Description : Definition of the DXGL wrapper for ID3D11RasterizerState


#include "RenderDll_precompiled.h"
#include "CCryDXGLRasterizerState.hpp"
#include "CCryDXGLDevice.hpp"
#include "../Implementation/GLState.hpp"
#include "../Implementation/GLDevice.hpp"


CCryDXGLRasterizerState::CCryDXGLRasterizerState(const D3D11_RASTERIZER_DESC& kDesc, CCryDXGLDevice* pDevice)
    : CCryDXGLDeviceChild(pDevice)
    , m_kDesc(kDesc)
    , m_pGLState(new NCryOpenGL::SRasterizerState)
{
    DXGL_INITIALIZE_INTERFACE(D3D11RasterizerState)
}

CCryDXGLRasterizerState::~CCryDXGLRasterizerState()
{
    delete m_pGLState;
}

bool CCryDXGLRasterizerState::Initialize(CCryDXGLDevice* pDevice, NCryOpenGL::CContext* pContext)
{
    return NCryOpenGL::InitializeRasterizerState(m_kDesc, *m_pGLState, pContext);
}

bool CCryDXGLRasterizerState::Apply(NCryOpenGL::CContext* pContext)
{
    return pContext->SetRasterizerState(*m_pGLState);
}


////////////////////////////////////////////////////////////////////////////////
// Implementation of ID3D11RasterizerState
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLRasterizerState::GetDesc(D3D11_RASTERIZER_DESC* pDesc)
{
    (*pDesc) = m_kDesc;
}
