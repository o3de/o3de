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

// Description : Definition of the DXGL wrapper for ID3D11BlendState


#include "RenderDll_precompiled.h"
#include "CCryDXGLBlendState.hpp"
#include "CCryDXGLDevice.hpp"
#include "../Implementation/GLState.hpp"
#include "../Implementation/GLDevice.hpp"

CCryDXGLBlendState::CCryDXGLBlendState(const D3D11_BLEND_DESC& kDesc, CCryDXGLDevice* pDevice)
    : CCryDXGLDeviceChild(pDevice)
    , m_kDesc(kDesc)
    , m_pGLState(new NCryOpenGL::SBlendState)
{
    DXGL_INITIALIZE_INTERFACE(D3D11BlendState)
}

CCryDXGLBlendState::~CCryDXGLBlendState()
{
    delete m_pGLState;
}

bool CCryDXGLBlendState::Initialize(CCryDXGLDevice* pDevice, NCryOpenGL::CContext* pContext)
{
    return NCryOpenGL::InitializeBlendState(m_kDesc, *m_pGLState, pContext);
}

bool CCryDXGLBlendState::Apply(NCryOpenGL::CContext* pContext)
{
    return pContext->SetBlendState(*m_pGLState);
}


////////////////////////////////////////////////////////////////////////////////
// Implementation of ID3D11BlendState
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLBlendState::GetDesc(D3D11_BLEND_DESC* pDesc)
{
    (*pDesc) = m_kDesc;
}