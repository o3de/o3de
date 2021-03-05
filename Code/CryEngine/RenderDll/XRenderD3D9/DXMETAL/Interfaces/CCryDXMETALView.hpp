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

// Description : Declaration of the DXGL wrapper for ID3D11View

#ifndef __CRYMETALGLVIEW__
#define __CRYMETALGLVIEW__

#include "CCryDXMETALDeviceChild.hpp"

class CCryDXGLResource;

class CCryDXGLView
    : public CCryDXGLDeviceChild
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLView, D3D11View)

    virtual ~CCryDXGLView();

    // Implementation of ID3D11View
    void GetResource(ID3D11Resource** ppResource);

#if !DXGL_FULL_EMULATION
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
    {
        if (SingleInterface<CCryDXGLView>::Query(this, riid, ppvObject))
        {
            return S_OK;
        }
        return CCryDXGLDeviceChild::QueryInterface(riid, ppvObject);
    }
#endif //!DXGL_FULL_EMULATION
protected:
    CCryDXGLView(CCryDXGLResource* pResource, CCryDXGLDevice* pDevice);

    _smart_ptr<CCryDXGLResource> m_spResource;
};

#endif //__CRYMETALGLVIEW__
