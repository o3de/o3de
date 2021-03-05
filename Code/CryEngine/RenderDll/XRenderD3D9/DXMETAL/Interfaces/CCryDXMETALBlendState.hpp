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

// Description : Declaration of the DXGL wrapper for ID3D11BlendState

#ifndef __CRYMETALGLBLENDSTATE__
#define __CRYMETALGLBLENDSTATE__

#include "CCryDXMETALDeviceChild.hpp"

namespace NCryMetal
{
    struct SBlendState;
    class CContext;
}

class CCryDXGLBlendState
    : public CCryDXGLDeviceChild
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLBlendState, D3D11BlendState)

    CCryDXGLBlendState(const D3D11_BLEND_DESC& kDesc, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLBlendState();

    bool Initialize(CCryDXGLDevice* pDevice);
    bool Apply(NCryMetal::CContext* pContext);

    // Implementation of ID3D11BlendState
    void GetDesc(D3D11_BLEND_DESC* pDesc);
protected:
    D3D11_BLEND_DESC m_kDesc;
    NCryMetal::SBlendState* m_pGLState;
};

#endif //__CRYMETALGLBLENDSTATE__

