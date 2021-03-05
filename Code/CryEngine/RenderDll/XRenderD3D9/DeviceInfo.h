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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DEVICEINFO_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DEVICEINFO_H
#pragma once

#if defined(SUPPORT_DEVICE_INFO)

#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
namespace DeviceInfoInternal
{
    struct MsgQueueItem
    {
        MsgQueueItem(ESystemEvent _event, UINT_PTR _wParam, UINT_PTR _lParam)
            : event(_event)
            , wParam(_wParam)
            , lParam(_lParam) {}
        ESystemEvent event;
        UINT_PTR wParam;
        UINT_PTR lParam;
    };

    typedef std::vector<MsgQueueItem> MsgQueue;
}
#endif

typedef HRESULT (CALLBACK * OnCreateDeviceCallback)(D3DDevice*);
typedef HWND (* CreateWindowCallback)();

struct DeviceInfo
{
    DeviceInfo();
    bool IsOk() const { return m_pFactory != 0 && m_pAdapter != 0 && m_pDevice != 0 && m_pContext != 0 && m_pSwapChain != 0 && m_pBackbufferRTVs.size() != 0; }
    void Release();

    bool CreateDevice(bool windowed, int width, int height, int backbufferWidth, int backbufferHeight, int zbpp, OnCreateDeviceCallback pCreateDeviceCallback, CreateWindowCallback pCreateWindowCallback);
    bool CreateViews();
    void SnapSettings();
    void ResizeDXGIBuffers();

#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
    void OnActivate(UINT_PTR wParam, UINT_PTR lParam);
    void PushSystemEvent(ESystemEvent event, UINT_PTR wParam, UINT_PTR lParam);
    void ProcessSystemEventQueue();
#endif

#if defined(WIN32) || defined(WIN64)
    void EnforceFullscreenPreemption();
    #ifdef CRY_INTEGRATE_DX12
    void WaitForGPUFrames();
    #endif
#endif

    // Properties
    DXGIFactory* Factory() const { return m_pFactory; }
    DXGIAdapter* Adapter() const { return m_pAdapter; }
    DXGIOutput* Output() const { return m_pOutput; }
    D3DDevice* Device() const { return m_pDevice; }
    D3DDeviceContext* Context() const { return m_pContext; }
    DXGISwapChain* SwapChain() const { return m_pSwapChain; }
    D3DSurface* BackbufferRTV() const { return m_pBackbufferRTVs[m_pCurrentBackBufferRTVIndex]; }
    std::vector<D3DSurface*> BackbufferRTVs() const { return m_pBackbufferRTVs; }

    const DXGI_ADAPTER_DESC1& AdapterDesc() { return m_adapterDesc; }
    DXGI_SWAP_CHAIN_DESC& SwapChainDesc() { return m_swapChainDesc; }
    const DXGI_RATIONAL& RefreshRate() const { return m_refreshRate; }

    D3D_DRIVER_TYPE DriverType() const { return m_driverType; }
    unsigned int CreationFlags() const { return m_creationFlags; }
    D3D_FEATURE_LEVEL FeatureLevel() const { return m_featureLevel; }
    D3DFormat AutoDepthStencilFmt() const { return m_autoDepthStencilFmt; }

    unsigned int OutputIndex() { return m_outputIndex; }
    unsigned int& SyncInterval() { return m_syncInterval; }
    unsigned int PresentFlags() const { return m_presentFlags; }

protected:
#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
    void ProcessSystemEvent(ESystemEvent event, UINT_PTR wParam, UINT_PTR lParam);
#endif

protected:
    DXGIFactory* m_pFactory;
    DXGIAdapter* m_pAdapter;
    DXGIOutput* m_pOutput;
    D3DDevice* m_pDevice;
    D3DDeviceContext* m_pContext;
    DXGISwapChain* m_pSwapChain;
    std::vector<D3DSurface*> m_pBackbufferRTVs;
    unsigned int m_pCurrentBackBufferRTVIndex;

    DXGI_ADAPTER_DESC1 m_adapterDesc;
    DXGI_SWAP_CHAIN_DESC m_swapChainDesc;
    DXGI_RATIONAL m_refreshRate;
    DXGI_RATIONAL m_desktopRefreshRate;

    DXGI_ADAPTER_FLAG m_adapterFlag;
    D3D_DRIVER_TYPE m_driverType;
    unsigned int m_creationFlags;
    D3D_FEATURE_LEVEL m_featureLevel;
    D3DFormat m_autoDepthStencilFmt;

    unsigned int m_outputIndex;
    unsigned int m_syncInterval;
    unsigned int m_presentFlags;

    bool m_activated;
    bool m_activatedMT;
#ifdef  CRY_INTEGRATE_DX12
    HANDLE m_frameLatencyWaitableObject;
#endif

#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
    CryCriticalSection m_msgQueueLock;
    DeviceInfoInternal::MsgQueue m_msgQueue;
#endif
};

#endif // #if defined(SUPPORT_DEVICE_INFO)

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DEVICEINFO_H
