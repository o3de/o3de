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

//  Description : Declaration of the DXGL wrapper for IDXGIAdapter

#ifndef __CRYMETALGLGIADAPTER__
#define __CRYMETALGLGIADAPTER__

#include "CCryDXMETALGIObject.hpp"

namespace NCryMetal
{
    struct SAdapter;
}

class CCryDXGLGIFactory;
class CCryDXGLGIOutput;

class CCryDXGLGIAdapter
    : public CCryDXGLGIObject
{
public:
#if DXGL_FULL_EMULATION
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLGIAdapter, DXGIAdapter)
#endif //DXGL_FULL_EMULATION
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLGIAdapter, DXGIAdapter1)

    CCryDXGLGIAdapter(CCryDXGLGIFactory* pFactory, NCryMetal::SAdapter* pGLAdapter);
    virtual ~CCryDXGLGIAdapter();

    bool Initialize();
    D3D_FEATURE_LEVEL GetSupportedFeatureLevel();
    NCryMetal::SAdapter* GetGLAdapter();

    // IDXGIObject overrides
    HRESULT GetParent(REFIID riid, void** ppParent);

    // IDXGIAdapter implementation
    HRESULT EnumOutputs(UINT Output, IDXGIOutput** ppOutput);
    HRESULT GetDesc(DXGI_ADAPTER_DESC* pDesc);
    HRESULT CheckInterfaceSupport(REFGUID InterfaceName, LARGE_INTEGER* pUMDVersion);

    // IDXGIAdapter1 implementation
    HRESULT GetDesc1(DXGI_ADAPTER_DESC1* pDesc);
protected:
    std::vector<_smart_ptr<CCryDXGLGIOutput> > m_kOutputs;
    _smart_ptr<CCryDXGLGIFactory> m_spFactory;

    _smart_ptr<NCryMetal::SAdapter> m_spGLAdapter;
    DXGI_ADAPTER_DESC m_kDesc;
    DXGI_ADAPTER_DESC1 m_kDesc1;
    D3D_FEATURE_LEVEL m_eSupportedFeatureLevel;
};

#endif //__CRYMETALGLGIADAPTER__
