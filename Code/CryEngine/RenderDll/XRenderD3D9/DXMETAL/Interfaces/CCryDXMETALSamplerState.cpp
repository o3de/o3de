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

// Description : Definition of the DXGL wrapper for ID3D11SamplerState

#include "RenderDll_precompiled.h"
#include "CCryDXMETALSamplerState.hpp"
#include "CCryDXMETALDevice.hpp"
#include "../Implementation/GLState.hpp"
#include "../Implementation/MetalDevice.hpp"


CCryDXGLSamplerState::CCryDXGLSamplerState(const D3D11_SAMPLER_DESC& kDesc, CCryDXGLDevice* pDevice)
    : CCryDXGLDeviceChild(pDevice)
    , m_kDesc(kDesc)
    , m_MetalSamplerState(nil)
    , m_MetalSamplerDescriptor(nil)
{
    DXGL_INITIALIZE_INTERFACE(D3D11SamplerState)
}

CCryDXGLSamplerState::~CCryDXGLSamplerState()
{
    if (m_MetalSamplerDescriptor)
    {
        [m_MetalSamplerDescriptor release];
        m_MetalSamplerDescriptor = nil;
    }
    
    if (m_MetalSamplerState)
    {
        [m_MetalSamplerState release];
        m_MetalSamplerState = nil;
    }
}

bool CCryDXGLSamplerState::Initialize(CCryDXGLDevice* pDevice)
{
    m_pDevice = pDevice;
    m_MetalSamplerDescriptor = [[MTLSamplerDescriptor alloc] init];
    return NCryMetal::InitializeSamplerState(m_kDesc, m_MetalSamplerState, m_MetalSamplerDescriptor, pDevice->GetGLDevice());
}

void CCryDXGLSamplerState::Apply(uint32 uStage, uint32 uSlot, NCryMetal::CContext* pContext)
{
    pContext->SetSampler(m_MetalSamplerState, uStage, uSlot);
}

void CCryDXGLSamplerState::SetLodMinClamp(float lodMinClamp)
{
    //You can either release the MTLSamplerDescriptor object or modify its property values and reuse it to create more MTLSamplerState objects.
    //The descriptor's properties are only used during object creation; once created the behavior of a sampler state object is fixed and cannot be changed.
    //Hence we delete the old sampler state and create a new one if any property needs to be changed.
    if (m_MetalSamplerState)
    {
        [m_MetalSamplerState release];
        m_MetalSamplerState = nil;
    }
    NCryMetal::SetLodMinClamp(m_MetalSamplerState, m_MetalSamplerDescriptor, lodMinClamp, m_pDevice->GetGLDevice());
}

////////////////////////////////////////////////////////////////
// Implementation of ID3D11SamplerState
////////////////////////////////////////////////////////////////

void CCryDXGLSamplerState::GetDesc(D3D11_SAMPLER_DESC* pDesc)
{
    (*pDesc) = m_kDesc;
}
