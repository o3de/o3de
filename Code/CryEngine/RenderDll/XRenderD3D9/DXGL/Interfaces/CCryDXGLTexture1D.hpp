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

// Description : Declaration of the DXGL wrapper for ID3D11Texture1D


#ifndef __CRYDXGLTEXTURE1D__
#define __CRYDXGLTEXTURE1D__

#include "CCryDXGLTextureBase.hpp"


class CCryDXGLTexture1D
    : public CCryDXGLTextureBase
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLTexture1D, D3D11Texture1D)

    CCryDXGLTexture1D(const D3D11_TEXTURE1D_DESC& kDesc, NCryOpenGL::STexture* pGLTexture, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLTexture1D();

    // Implementation of ID3D11Texture1D
    void GetDesc(D3D11_TEXTURE1D_DESC* pDesc);

#if !DXGL_FULL_EMULATION
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
    {
        if (SingleInterface<CCryDXGLTexture1D>::Query(this, riid, ppvObject))
        {
            return S_OK;
        }
        return CCryDXGLTextureBase::QueryInterface(riid, ppvObject);
    }
#endif //!DXGL_FULL_EMULATION
private:
    D3D11_TEXTURE1D_DESC m_kDesc;
};

#endif //__CRYDXGLTEXTURE1D__
