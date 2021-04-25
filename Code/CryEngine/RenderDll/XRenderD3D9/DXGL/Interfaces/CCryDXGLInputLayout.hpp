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

// Description : Declaration of the DXGL wrapper for ID3D11InputLayout


#ifndef __CRYDXGLINPUTLAYOUT__
#define __CRYDXGLINPUTLAYOUT__

#include "CCryDXGLDeviceChild.hpp"

namespace NCryOpenGL
{
    struct SInputLayout;
}

class CCryDXGLInputLayout
    : public CCryDXGLDeviceChild
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLInputLayout, D3D11InputLayout)

    CCryDXGLInputLayout(NCryOpenGL::SInputLayout* pGLLayout, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLInputLayout();

    NCryOpenGL::SInputLayout* GetGLLayout();
private:
    _smart_ptr<NCryOpenGL::SInputLayout> m_spGLLayout;
};

#endif //__CRYDXGLINPUTLAYOUT__
