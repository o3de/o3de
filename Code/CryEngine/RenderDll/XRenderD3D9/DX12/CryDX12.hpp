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
#pragma once

#include "CryDX12Legacy.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "API/DX12Base.hpp"
#include "API/DX12PSO.hpp"
#include "Misc/SCryDX11PipelineState.hpp"

#include "XRenderD3D9/DX12/GI/CCryDX12GIFactory.hpp"
#include "XRenderD3D9/DX12/GI/CCryDX12SwapChain.hpp"
#include "XRenderD3D9/DX12/Device/CCryDX12Device.hpp"
#include "XRenderD3D9/DX12/Device/CCryDX12DeviceContext.hpp"
typedef IDXGIDevice1            DXGIDevice;
typedef IDXGIAdapter1           DXGIAdapter;
typedef CCryDX12GIFactory       DXGIFactory;
typedef IDXGIOutput             DXGIOutput;
typedef CCryDX12SwapChain       DXGISwapChain;
typedef CCryDX12Device          D3DDevice;
typedef CCryDX12DeviceContext   D3DDeviceContext;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI DX12CreateDXGIFactory1(REFIID riid, void** ppFactory);

HRESULT WINAPI DX12CreateDevice(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext);
