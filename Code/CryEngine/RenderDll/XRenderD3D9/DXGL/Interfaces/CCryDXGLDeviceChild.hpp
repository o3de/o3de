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

// Description: Declaration of the DXGL wrapper for ID3D11DeviceChild


#ifndef __CRYDXGLDEVICECHILD__
#define __CRYDXGLDEVICECHILD__

#include "CCryDXGLBase.hpp"

class CCryDXGLDevice;

class CCryDXGLDeviceChild
    : public CCryDXGLBase
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLDeviceChild, D3D11DeviceChild)

    CCryDXGLDeviceChild(CCryDXGLDevice* pDevice = NULL);
    virtual ~CCryDXGLDeviceChild();

    void SetDevice(CCryDXGLDevice* pDevice);

    // ID3D11DeviceChild implementation
    void STDMETHODCALLTYPE GetDevice(ID3D11Device** ppDevice);
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData);
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData);
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData);

#if !DXGL_FULL_EMULATION
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
    {
        if (SingleInterface<CCryDXGLDeviceChild>::Query(this, riid, ppvObject))
        {
            return S_OK;
        }
        return CCryDXGLBase::QueryInterface(riid, ppvObject);
    }
#endif //!DXGL_FULL_EMULATION
protected:
    CCryDXGLDevice* m_pDevice;
    CCryDXGLPrivateDataContainer m_kPrivateDataContainer;
};

#if !DXGL_FULL_EMULATION
struct ID3D11Counter
    : CCryDXGLDeviceChild {};
struct ID3D11ClassLinkage
    : CCryDXGLDeviceChild {};
#endif //!DXGL_FULL_EMULATION

#endif //__CRYDXGLDEVICECHILD__
