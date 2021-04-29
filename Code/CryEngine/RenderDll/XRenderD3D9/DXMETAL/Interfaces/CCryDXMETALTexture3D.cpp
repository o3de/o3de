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

// Description : Definition of the DXGL wrapper for ID3D11Texture3D

#include "RenderDll_precompiled.h"
#include "CCryDXMETALTexture3D.hpp"


CCryDXGLTexture3D::CCryDXGLTexture3D(const D3D11_TEXTURE3D_DESC& kDesc, NCryMetal::STexture* pGLTexture, CCryDXGLDevice* pDevice)
    : CCryDXGLTextureBase(D3D11_RESOURCE_DIMENSION_TEXTURE3D, pGLTexture, pDevice)
    , m_kDesc(kDesc)
{
    DXGL_INITIALIZE_INTERFACE(D3D11Texture3D)
}

CCryDXGLTexture3D::~CCryDXGLTexture3D()
{
}


////////////////////////////////////////////////////////////////////////////////
// Implementation of ID3D11Texture3D
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLTexture3D::GetDesc(D3D11_TEXTURE3D_DESC* pDesc)
{
    *pDesc = m_kDesc;
}
