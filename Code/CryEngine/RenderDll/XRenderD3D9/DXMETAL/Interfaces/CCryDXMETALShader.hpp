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

// Description : Declaration of the DXGL wrapper for D3D11 shader interfaces

#ifndef __CRYMETALGLSHADER__
#define __CRYMETALGLSHADER__

#include "CCryDXMETALDeviceChild.hpp"

namespace NCryMetal
{
    struct SShader;
}

class CCryDXGLShader
    : public CCryDXGLDeviceChild
{
public:
    CCryDXGLShader(NCryMetal::SShader* pGLShader, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLShader();

    NCryMetal::SShader* GetGLShader();
private:
    _smart_ptr<NCryMetal::SShader> m_spGLShader;
};

class CCryDXGLVertexShader
    : public CCryDXGLShader
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLVertexShader, D3D11VertexShader)

    CCryDXGLVertexShader(NCryMetal::SShader* pGLShader, CCryDXGLDevice* pDevice)
        : CCryDXGLShader(pGLShader, pDevice)
    {
        DXGL_INITIALIZE_INTERFACE(D3D11VertexShader)
    }
};

class CCryDXGLHullShader
    : public CCryDXGLShader
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLHullShader, D3D11HullShader)

    CCryDXGLHullShader(NCryMetal::SShader* pGLShader, CCryDXGLDevice* pDevice)
        : CCryDXGLShader(pGLShader, pDevice)
    {
        DXGL_INITIALIZE_INTERFACE(D3D11HullShader)
    }
};

class CCryDXGLDomainShader
    : public CCryDXGLShader
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLDomainShader, D3D11DomainShader)

    CCryDXGLDomainShader(NCryMetal::SShader* pGLShader, CCryDXGLDevice* pDevice)
        : CCryDXGLShader(pGLShader, pDevice)
    {
        DXGL_INITIALIZE_INTERFACE(D3D11DomainShader)
    }
};

class CCryDXGLGeometryShader
    : public CCryDXGLShader
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLGeometryShader, D3D11GeometryShader)

    CCryDXGLGeometryShader(NCryMetal::SShader* pGLShader, CCryDXGLDevice* pDevice)
        : CCryDXGLShader(pGLShader, pDevice)
    {
        DXGL_INITIALIZE_INTERFACE(D3D11GeometryShader)
    }
};

class CCryDXGLPixelShader
    : public CCryDXGLShader
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLPixelShader, D3D11PixelShader)

    CCryDXGLPixelShader(NCryMetal::SShader* pGLShader, CCryDXGLDevice* pDevice)
        : CCryDXGLShader(pGLShader, pDevice)
    {
        DXGL_INITIALIZE_INTERFACE(D3D11PixelShader)
    }
};

class CCryDXGLComputeShader
    : public CCryDXGLShader
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLComputeShader, D3D11ComputeShader)

    CCryDXGLComputeShader(NCryMetal::SShader* pGLShader, CCryDXGLDevice* pDevice)
        : CCryDXGLShader(pGLShader, pDevice)
    {
        DXGL_INITIALIZE_INTERFACE(D3D11ComputeShader)
    }
};

#endif  //__CRYMETALGLSHADER__
