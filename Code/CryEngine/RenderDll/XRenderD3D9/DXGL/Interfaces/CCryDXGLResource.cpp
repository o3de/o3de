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


#include "RenderDll_precompiled.h"
#include "CCryDXGLResource.hpp"
#include "../Implementation/GLResource.hpp"

CCryDXGLResource::CCryDXGLResource(D3D11_RESOURCE_DIMENSION eDimension, NCryOpenGL::SResource* pGLResource, CCryDXGLDevice* pDevice)
    : CCryDXGLDeviceChild(pDevice)
    , m_spGLResource(pGLResource)
    , m_eDimension(eDimension)
{
    DXGL_INITIALIZE_INTERFACE(D3D11Resource)
}

CCryDXGLResource::~CCryDXGLResource()
{
}


////////////////////////////////////////////////////////////////////////////////
// Implementation of ID3D11Resource
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLResource::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    *pResourceDimension = m_eDimension;
}

void CCryDXGLResource::SetEvictionPriority(UINT EvictionPriority)
{
    DXGL_NOT_IMPLEMENTED
}

UINT CCryDXGLResource::GetEvictionPriority(void)
{
    DXGL_NOT_IMPLEMENTED
    return 0;
}