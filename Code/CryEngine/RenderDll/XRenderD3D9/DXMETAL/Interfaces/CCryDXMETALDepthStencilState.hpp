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


//  Description: Declaration of the DXGL wrapper for ID3D11DepthStencilState

#ifndef __CRYMETALGLDEPTHSTENCILSTATE__
#define __CRYMETALGLDEPTHSTENCILSTATE__

#include "CCryDXMETALDeviceChild.hpp"

@protocol MTLDepthStencilState;

namespace NCryMetal
{
    class CContext;
}

class CCryDXGLDepthStencilState
    : public CCryDXGLDeviceChild
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLDepthStencilState, D3D11DepthStencilState)

    CCryDXGLDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& kDesc, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLDepthStencilState();

    bool Initialize(CCryDXGLDevice* pDevice);
    bool Apply(uint32 uStencilReference, NCryMetal::CContext* pContext);

    // Implementation of ID3D11DepthStencilState
    void GetDesc(D3D11_DEPTH_STENCIL_DESC* pDesc);
protected:
    D3D11_DEPTH_STENCIL_DESC m_kDesc;
    id<MTLDepthStencilState>        m_MetalDepthStencilState;
};

#endif //__CRYMETALGLDEPTHSTENCILSTATE__

