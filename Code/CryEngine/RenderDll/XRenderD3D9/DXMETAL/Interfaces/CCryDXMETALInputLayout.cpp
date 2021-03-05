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

// Description : Definition of the DXGL wrapper for ID3D11InputLayout

#include "RenderDll_precompiled.h"
#include "CCryDXMETALInputLayout.hpp"
#include "../Implementation/GLShader.hpp"


CCryDXGLInputLayout::CCryDXGLInputLayout(NCryMetal::SInputLayout* pGLLayout, CCryDXGLDevice* pDevice)
    : CCryDXGLDeviceChild(pDevice)
    , m_spGLLayout(pGLLayout)
{
    DXGL_INITIALIZE_INTERFACE(D3D11InputLayout)
}

CCryDXGLInputLayout::~CCryDXGLInputLayout()
{
}

NCryMetal::SInputLayout* CCryDXGLInputLayout::GetGLLayout()
{
    return m_spGLLayout;
}