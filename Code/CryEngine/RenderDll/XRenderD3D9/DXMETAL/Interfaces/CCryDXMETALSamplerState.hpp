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

// Description : Declaration of the DXGL wrapper for ID3D11SamplerState

#ifndef __CRYMETALGLSAMPLERSTATE__
#define __CRYMETALGLSAMPLERSTATE__

#include "CCryDXMETALDeviceChild.hpp"


@protocol MTLSamplerState;
#import <Metal/MTLSampler.h>

namespace NCryMetal
{
    struct SSamplerState;
    class CContext;
}

class CCryDXGLSamplerState
    : public CCryDXGLDeviceChild
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLSamplerState, D3D11SamplerState)

    CCryDXGLSamplerState(const D3D11_SAMPLER_DESC& kDesc, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLSamplerState();

    bool Initialize(CCryDXGLDevice* pDevice);
    void Apply(uint32 uStage, uint32 uSlot, NCryMetal::CContext* pContext);

    void SetLodMinClamp(float lodMinClamp);
    void GetDesc(D3D11_SAMPLER_DESC* pDesc);
protected:
    D3D11_SAMPLER_DESC m_kDesc;
    
    id<MTLSamplerState> m_MetalSamplerState;
    MTLSamplerDescriptor* m_MetalSamplerDescriptor;
    CCryDXGLDevice* m_pDevice;
};

#endif //__CRYMETALGLSAMPLERSTATE__
