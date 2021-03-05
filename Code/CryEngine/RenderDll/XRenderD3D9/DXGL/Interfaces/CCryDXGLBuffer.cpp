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

// Description : Definition of the DXGL wrapper for ID3D11Buffer


#include "RenderDll_precompiled.h"
#include "CCryDXGLBuffer.hpp"
#include "CCryDXGLDeviceContext.hpp"
#include "../Implementation/GLResource.hpp"

CCryDXGLBuffer::CCryDXGLBuffer(const D3D11_BUFFER_DESC& kDesc, NCryOpenGL::SBuffer* pGLBuffer, CCryDXGLDevice* pDevice)
    : CCryDXGLResource(D3D11_RESOURCE_DIMENSION_BUFFER, pGLBuffer, pDevice)
    , m_kDesc(kDesc)
{
    DXGL_INITIALIZE_INTERFACE(D3D11Buffer)
}

CCryDXGLBuffer::~CCryDXGLBuffer()
{
}

NCryOpenGL::SBuffer* CCryDXGLBuffer::GetGLBuffer()
{
    return static_cast<NCryOpenGL::SBuffer*>(m_spGLResource.get());
}


////////////////////////////////////////////////////////////////////////////////
// ID3D11Buffer implementation
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLBuffer::GetDesc(D3D11_BUFFER_DESC* pDesc)
{
    (*pDesc) = m_kDesc;
}
