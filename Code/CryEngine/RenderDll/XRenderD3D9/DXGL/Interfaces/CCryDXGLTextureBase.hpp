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

// Description : Declaration of the DXGL common base class for textures


#ifndef __CRYDXGLTEXTUREBASE__
#define __CRYDXGLTEXTUREBASE__

#include "CCryDXGLResource.hpp"

namespace NCryOpenGL
{
    struct STexture;
};

class CCryDXGLTextureBase
    : public CCryDXGLResource
{
public:
    CCryDXGLTextureBase(D3D11_RESOURCE_DIMENSION eDimension, NCryOpenGL::STexture* pGLTexture, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLTextureBase();

    NCryOpenGL::STexture* GetGLTexture();
};

#endif //__CRYDXGLTEXTUREBASE__
