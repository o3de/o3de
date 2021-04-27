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

// Description : Declaration of the DXGL wrapper for ID3D11RasterizerState


#ifndef __CRYDXGLRASTERIZERSTATE__
#define __CRYDXGLRASTERIZERSTATE__

#include "CCryDXGLDeviceChild.hpp"

namespace NCryOpenGL
{
    struct SRasterizerState;
    class CContext;
};

class CCryDXGLRasterizerState
    : public CCryDXGLDeviceChild
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLRasterizerState, D3D11RasterizerState)

    CCryDXGLRasterizerState(const D3D11_RASTERIZER_DESC& kDesc, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLRasterizerState();

    bool Initialize(CCryDXGLDevice* pDevice, NCryOpenGL::CContext* pContext);
    bool Apply(NCryOpenGL::CContext* pContext);

    // Implementation of ID3D11RasterizerState
    void GetDesc(D3D11_RASTERIZER_DESC* pDesc);
protected:
    D3D11_RASTERIZER_DESC m_kDesc;
    NCryOpenGL::SRasterizerState* m_pGLState;
};

#endif //__CRYDXGLRASTERIZERSTATE__
