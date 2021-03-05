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

// Description : Definition of the DXGL wrapper for ID3D11UnorderedAccessView

#include "RenderDll_precompiled.h"
#include "CCryDXMETALUnorderedAccessView.hpp"
#include "../Implementation/GLResource.hpp"

CCryDXGLUnorderedAccessView::CCryDXGLUnorderedAccessView(CCryDXGLResource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC& kDesc, CCryDXGLDevice* pDevice)
    : CCryDXGLView(pResource, pDevice)
    , m_kDesc(kDesc)
{
    DXGL_INITIALIZE_INTERFACE(D3D11UnorderedAccessView)
}

CCryDXGLUnorderedAccessView::~CCryDXGLUnorderedAccessView()
{
}

NCryMetal::SBuffer* CCryDXGLUnorderedAccessView::GetGLBuffer()
{
    return static_cast<NCryMetal::SBuffer*>(m_spResource->GetGLResource());
}

NCryMetal::STexture* CCryDXGLUnorderedAccessView::GetGLTexture()
{
    return static_cast<NCryMetal::STexture*>(m_spResource->GetGLResource());
}

////////////////////////////////////////////////////////////////
// Implementation of ID3D11UnorderedAccessView
////////////////////////////////////////////////////////////////

void CCryDXGLUnorderedAccessView::GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc)
{
    *pDesc = m_kDesc;
}