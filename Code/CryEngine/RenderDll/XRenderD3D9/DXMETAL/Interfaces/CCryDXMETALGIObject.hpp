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

// Description : Declaration of the DXGL wrapper for IDXGIObject

#ifndef __CRYMETALGLGIOBJECT__
#define __CRYMETALGLGIOBJECT__

#include "CCryDXMETALBase.hpp"


class CCryDXGLGIObject
    : public CCryDXGLBase
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLGIObject, DXGIObject)

    CCryDXGLGIObject();
    virtual ~CCryDXGLGIObject();

    // IDXGIObject implementation
    HRESULT SetPrivateData(REFGUID Name, UINT DataSize, const void* pData);
    HRESULT SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown);
    HRESULT GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData);
    HRESULT GetParent(REFIID riid, void** ppParent);

#if !DXGL_FULL_EMULATION
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
    {
        if (SingleInterface<CCryDXGLGIObject>::Query(this, riid, ppvObject))
        {
            return S_OK;
        }
        return CCryDXGLBase::QueryInterface(riid, ppvObject);
    }
#endif //!DXGL_FULL_EMULATION

protected:
    CCryDXGLPrivateDataContainer m_kPrivateDataContainer;
};

#endif //__CRYMETALGLGIOBJECT__
