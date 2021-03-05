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

//  Description: Definition of the DXGL wrapper for ID3D11DeviceChild

#include "RenderDll_precompiled.h"
#include "CCryDXMETALDeviceChild.hpp"
#include "CCryDXMETALDevice.hpp"

CCryDXGLDeviceChild::CCryDXGLDeviceChild(CCryDXGLDevice* pDevice)
    : m_pDevice(pDevice)
{
    DXGL_INITIALIZE_INTERFACE(D3D11DeviceChild)
    if (m_pDevice != NULL)
    {
        m_pDevice->AddRef();
    }
}

CCryDXGLDeviceChild::~CCryDXGLDeviceChild()
{
    if (m_pDevice != NULL)
    {
        m_pDevice->Release();
    }
}

void CCryDXGLDeviceChild::SetDevice(CCryDXGLDevice* pDevice)
{
    if (m_pDevice != pDevice)
    {
        if (m_pDevice != NULL)
        {
            m_pDevice->Release();
        }
        m_pDevice = pDevice;
        if (pDevice != NULL)
        {
            m_pDevice->AddRef();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// ID3D11DeviceChild implementation
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLDeviceChild::GetDevice(ID3D11Device** ppDevice)
{
    CCryDXGLDevice::ToInterface(ppDevice, m_pDevice);
}

HRESULT CCryDXGLDeviceChild::GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData)
{
    return m_kPrivateDataContainer.GetPrivateData(guid, pDataSize, pData);
}

HRESULT CCryDXGLDeviceChild::SetPrivateData(REFGUID guid, UINT DataSize, const void* pData)
{
    return m_kPrivateDataContainer.SetPrivateData(guid, DataSize, pData);
}

HRESULT CCryDXGLDeviceChild::SetPrivateDataInterface(REFGUID guid, const IUnknown* pData)
{
    return m_kPrivateDataContainer.SetPrivateDataInterface(guid, pData);
}
