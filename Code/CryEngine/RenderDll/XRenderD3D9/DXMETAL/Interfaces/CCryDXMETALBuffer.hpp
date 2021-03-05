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

// Description : Declaration of the DXGL wrapper for ID3D11Buffer

#ifndef __CRYMETALGLBUFFER__
#define __CRYMETALGLBUFFER__

#include "CCryDXMETALResource.hpp"

namespace NCryMetal
{
    struct SBuffer;
}

class CCryDXGLBuffer
    : public CCryDXGLResource
{
public:
    DXGL_IMPLEMENT_INTERFACE(CCryDXGLBuffer, D3D11Buffer)

    CCryDXGLBuffer(const D3D11_BUFFER_DESC& kDesc, NCryMetal::SBuffer* pGLBuffer, CCryDXGLDevice* pDevice);
    virtual ~CCryDXGLBuffer();

    // ID3D11Buffer implementation
    void GetDesc(D3D11_BUFFER_DESC* pDesc);

    NCryMetal::SBuffer* GetGLBuffer();
private:
    D3D11_BUFFER_DESC m_kDesc;
};

#endif //__CRYMETALGLBUFFER__