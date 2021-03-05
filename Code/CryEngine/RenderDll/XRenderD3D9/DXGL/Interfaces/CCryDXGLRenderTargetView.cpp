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

// Description : Definition of the DXGL wrapper for ID3D11RenderTargetView


#include "RenderDll_precompiled.h"
#include "CCryDXGLDevice.hpp"
#include "CCryDXGLRenderTargetView.hpp"
#include "CCryDXGLResource.hpp"
#include "../Implementation/GLDevice.hpp"
#include "../Implementation/GLView.hpp"


CCryDXGLRenderTargetView::CCryDXGLRenderTargetView(CCryDXGLResource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC& kDesc, CCryDXGLDevice* pDevice)
    : CCryDXGLView(pResource, pDevice)
    , m_kDesc(kDesc)
{
    DXGL_INITIALIZE_INTERFACE(D3D11RenderTargetView)
}

CCryDXGLRenderTargetView::~CCryDXGLRenderTargetView()
{
}

bool CCryDXGLRenderTargetView::Initialize(NCryOpenGL::CContext* pContext)
{
    D3D11_RESOURCE_DIMENSION eDimension;
    m_spResource->GetType(&eDimension);
    m_spGLView = NCryOpenGL::CreateRenderTargetView(m_spResource->GetGLResource(), eDimension, m_kDesc, pContext);
    return m_spGLView != NULL;
}

NCryOpenGL::SOutputMergerView* CCryDXGLRenderTargetView::GetGLView()
{
    return m_spGLView;
}


////////////////////////////////////////////////////////////////
// Implementation of ID3D11RenderTargetView
////////////////////////////////////////////////////////////////

void CCryDXGLRenderTargetView::GetDesc(D3D11_RENDER_TARGET_VIEW_DESC* pDesc)
{
    (*pDesc) = m_kDesc;
}