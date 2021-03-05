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

// Description : Declaration of the DXGL wrapper for ID3D11Resource

#ifndef __CRYMETALGLRESOURCE__
#define __CRYMETALGLRESOURCE__

#include "CCryDXMETALDeviceChild.hpp"

namespace NCryMetal
{
    class CDevice;
    struct SResource;
};

class CCryDXGLResource
    : public CCryDXGLDeviceChild
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLResource, D3D11Resource)

    virtual ~CCryDXGLResource();

    NCryMetal::SResource* GetGLResource();

    // Implementation of ID3D11Resource
    void GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension);
    void SetEvictionPriority(UINT EvictionPriority);
    UINT GetEvictionPriority(void);

#if !DXGL_FULL_EMULATION
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
    {
        if (SingleInterface<CCryDXGLResource>::Query(this, riid, ppvObject))
        {
            return S_OK;
        }
        return CCryDXGLDeviceChild::QueryInterface(riid, ppvObject);
    }
#endif //!DXGL_FULL_EMULATION
protected:
    CCryDXGLResource(D3D11_RESOURCE_DIMENSION eDimension, NCryMetal::SResource* pResource, CCryDXGLDevice* pDevice);
protected:
    _smart_ptr<NCryMetal::SResource> m_spGLResource;
    D3D11_RESOURCE_DIMENSION m_eDimension;
};

#endif //__CRYMETALGLRESOURCE__