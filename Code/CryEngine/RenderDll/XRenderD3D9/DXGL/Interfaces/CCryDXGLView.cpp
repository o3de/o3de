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

// Description : Definition of the DXGL wrapper for ID3D11View


#include "RenderDll_precompiled.h"
#include "CCryDXGLView.hpp"
#include "CCryDXGLResource.hpp"
#include "../Implementation/GLResource.hpp"


CCryDXGLView::CCryDXGLView(CCryDXGLResource* pResource, CCryDXGLDevice* pDevice)
    : CCryDXGLDeviceChild(pDevice)
    , m_spResource(pResource)
{
    DXGL_INITIALIZE_INTERFACE(D3D11View)
}

CCryDXGLView::~CCryDXGLView()
{
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of ID3D11View
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLView::GetResource(ID3D11Resource** ppResource)
{
    if (m_spResource != NULL)
    {
        m_spResource->AddRef();
    }
    CCryDXGLResource::ToInterface(ppResource, m_spResource);
}