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
#include "RenderDll_precompiled.h"
#include "CryDX12.hpp"

#include "GI/CCryDX12GIFactory.hpp"
#include "Device/CCryDX12Device.hpp"
#include "Device/CCryDX12DeviceContext.hpp"

HRESULT WINAPI DX12CreateDXGIFactory1([[maybe_unused]] REFIID riid, void** ppFactory)
{
    *ppFactory = CCryDX12GIFactory::Create();
    return *ppFactory ? 0 : -1;
}

HRESULT WINAPI DX12CreateDevice(
    IDXGIAdapter* pAdapter,
    [[maybe_unused]] D3D_DRIVER_TYPE DriverType,
    [[maybe_unused]] HMODULE Software,
    [[maybe_unused]] UINT Flags,
    [[maybe_unused]] CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    [[maybe_unused]] UINT FeatureLevels,
    [[maybe_unused]] UINT SDKVersion,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext)
{
    *ppDevice = CCryDX12Device::Create(pAdapter, pFeatureLevel);

    if (!*ppDevice)
    {
        return -1;
    }

    (*ppDevice)->GetImmediateContext(ppImmediateContext);

    if (!*ppImmediateContext)
    {
        return -1;
    }

    return 0;
}
