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

// Description : Declaration of the DXGL wrapper for ID3D11DepthStencilView

#ifndef __CRYMETALGLDEPTHSTENCILVIEW__
#define __CRYMETALGLDEPTHSTENCILVIEW__

#include "CCryDXMETALView.hpp"

namespace NCryMetal
{
    struct SOutputMergerView;
    class CContext;
}

class CCryDXGLDepthStencilView
    : public CCryDXGLView
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLDepthStencilView, D3D11DepthStencilView)

    CCryDXGLDepthStencilView(CCryDXGLResource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC& kDesc, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLDepthStencilView();

    bool Initialize(NCryMetal::CDevice* pDevice);
    NCryMetal::SOutputMergerView* GetGLView();

    // Implementation of ID3D11DepthStencilView
    void GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc);
protected:
    D3D11_DEPTH_STENCIL_VIEW_DESC m_kDesc;
    _smart_ptr<NCryMetal::SOutputMergerView> m_spGLView;
};

#endif //__CRYMETALGLDEPTHSTENCILVIEW__
