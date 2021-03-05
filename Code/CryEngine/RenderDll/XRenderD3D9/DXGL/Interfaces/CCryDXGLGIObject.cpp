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

// Description : Definition of the DXGL wrapper for IDXGIObject


#include "RenderDll_precompiled.h"
#include "CCryDXGLGIObject.hpp"
#include "../Implementation/GLCommon.hpp"


CCryDXGLGIObject::CCryDXGLGIObject()
{
    DXGL_INITIALIZE_INTERFACE(DXGIObject)
}

CCryDXGLGIObject::~CCryDXGLGIObject()
{
}


////////////////////////////////////////////////////////////////////////////////
// IDXGIObject implementation
////////////////////////////////////////////////////////////////////////////////

HRESULT CCryDXGLGIObject::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return m_kPrivateDataContainer.SetPrivateData(Name, DataSize, pData);
}

HRESULT CCryDXGLGIObject::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return m_kPrivateDataContainer.SetPrivateDataInterface(Name, pUnknown);
}

HRESULT CCryDXGLGIObject::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return m_kPrivateDataContainer.GetPrivateData(Name, pDataSize, pData);
}

HRESULT CCryDXGLGIObject::GetParent(REFIID riid, void** ppParent)
{
    DXGL_TODO("Implement if required")
    * ppParent = NULL;
    return E_FAIL;
}