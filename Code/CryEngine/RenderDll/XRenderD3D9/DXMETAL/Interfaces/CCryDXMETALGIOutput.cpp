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

// Description : Definition of the DXGL wrapper for IDXGIOutput

#include "RenderDll_precompiled.h"
#include "CCryDXMETALGIOutput.hpp"
#include "../Implementation/MetalDevice.hpp"


CCryDXGLGIOutput::CCryDXGLGIOutput()
{
    DXGL_INITIALIZE_INTERFACE(DXGIOutput)
}

CCryDXGLGIOutput::~CCryDXGLGIOutput()
{
}

bool CCryDXGLGIOutput::Initialize()
{
    ZeroMemory(&m_OutputDesc, sizeof(m_OutputDesc));
    mbstowcs(m_OutputDesc.DeviceName, "Metal Device", DXGL_ARRAY_SIZE(m_OutputDesc.DeviceName));
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// IDXGIOutput implementation
////////////////////////////////////////////////////////////////////////////////

HRESULT CCryDXGLGIOutput::GetDesc(DXGI_OUTPUT_DESC* pDesc)
{
    *pDesc = m_OutputDesc;
    return S_OK;
}

HRESULT CCryDXGLGIOutput::GetDisplayModeList(DXGI_FORMAT EnumFormat, UINT Flags, UINT* pNumModes, DXGI_MODE_DESC* pDesc)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLGIOutput::FindClosestMatchingMode(const DXGI_MODE_DESC* pModeToMatch, DXGI_MODE_DESC* pClosestMatch, IUnknown* pConcernedDevice)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLGIOutput::WaitForVBlank(void)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLGIOutput::TakeOwnership(IUnknown* pDevice, BOOL Exclusive)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

void CCryDXGLGIOutput::ReleaseOwnership(void)
{
    DXGL_NOT_IMPLEMENTED
}

HRESULT CCryDXGLGIOutput::GetGammaControlCapabilities(DXGI_GAMMA_CONTROL_CAPABILITIES* pGammaCaps)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLGIOutput::SetGammaControl(const DXGI_GAMMA_CONTROL* pArray)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLGIOutput::GetGammaControl(DXGI_GAMMA_CONTROL* pArray)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLGIOutput::SetDisplaySurface(IDXGISurface* pScanoutSurface)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLGIOutput::GetDisplaySurfaceData(IDXGISurface* pDestination)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT CCryDXGLGIOutput::GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
{
    DXGL_NOT_IMPLEMENTED
    return E_FAIL;
}
