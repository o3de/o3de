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

// Description : Declaration of the DXGL wrapper for ID3D11Query

#ifndef __CRYMETALGLQUERY__
#define __CRYMETALGLQUERY__

#include "CCryDXMETALDeviceChild.hpp"

namespace NCryMetal
{
    struct SQuery;
}

class CCryDXGLQuery
    : public CCryDXGLDeviceChild
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLQuery, D3D11Query)
#if DXGL_FULL_EMULATION
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLQuery, D3D11Asynchronous)
#endif //DXGL_FULL_EMULATION

    CCryDXGLQuery(const D3D11_QUERY_DESC& kDesc, NCryMetal::SQuery* pGLQuery, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLQuery();

    NCryMetal::SQuery* GetGLQuery();

    // ID3D11Asynchronous implementation
    UINT GetDataSize(void);

    // ID3D11Query implementation
    void GetDesc(D3D11_QUERY_DESC* pDesc);

#if !DXGL_FULL_EMULATION
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
    {
        if (SingleInterface<CCryDXGLQuery>::Query(this, riid, ppvObject))
        {
            return S_OK;
        }
        return CCryDXGLDeviceChild::QueryInterface(riid, ppvObject);
    }
#endif //!DXGL_FULL_EMULATION
private:
    D3D11_QUERY_DESC m_kDesc;
    _smart_ptr<NCryMetal::SQuery> m_spGLQuery;
};

#endif //__CRYMETALGLQUERY__
