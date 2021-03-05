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

#if defined(SUPPORT_DEVICE_INFO)

#include <AzFramework/API/ApplicationAPI.h>

enum
{
#if defined(WIN32) || defined(WIN64)
    DefaultBufferCount = 3
#else
    DefaultBufferCount = 2
#endif
};

static void InitSwapChain(DXGI_SWAP_CHAIN_DESC& desc, UINT width, UINT height, HWND hWnd, BOOL windowed)
{
    desc.BufferDesc.Width = width;
    desc.BufferDesc.Height = height;
    desc.BufferDesc.RefreshRate.Numerator = 0;
    desc.BufferDesc.RefreshRate.Denominator = 0;
#ifdef ANDROID
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
#else
    static ICVar* DolbyCvar = gEnv->pConsole->GetCVar("r_HDRDolby");
    if (DolbyCvar->GetIVal() == 1)
    {
        // Dolby Maui HDR PQ output format (10 bits)
        desc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
    }
    else
    {
        // Conventional SDR RGBA8 output buffer
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
#endif
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = gRenDev->CV_r_minimizeLatency > 0 ? 2 : DefaultBufferCount;
    desc.OutputWindow = hWnd;
    desc.Windowed = windowed ? 1 : 0;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#ifdef CRY_INTEGRATE_DX12
    desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
#endif
}

DeviceInfo::DeviceInfo()
    : m_pFactory(0)
    , m_pAdapter(0)
    , m_pOutput(0)
    , m_pDevice(0)
    , m_pContext(0)
    , m_pSwapChain(0)
    , m_adapterFlag(DXGI_ADAPTER_FLAG_NONE)
    , m_driverType(D3D_DRIVER_TYPE_NULL)
    , m_creationFlags(0)
    , m_featureLevel(D3D_FEATURE_LEVEL_9_1)
#ifdef CRY_USE_METAL
    , m_autoDepthStencilFmt(DXGI_FORMAT_R32G8X24_TYPELESS)
#else
    , m_autoDepthStencilFmt(DXGI_FORMAT_R24G8_TYPELESS)
#endif
    , m_outputIndex(0)
    , m_syncInterval(0)
    , m_presentFlags(0)
    , m_activated(true)
    , m_activatedMT(true)
#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
    , m_msgQueueLock()
    , m_msgQueue()
#endif
{
    memset(&m_adapterDesc, 0, sizeof(m_adapterDesc));
    memset(&m_swapChainDesc, 0, sizeof(m_swapChainDesc));
    memset(&m_refreshRate, 0, sizeof(m_refreshRate));
    memset(&m_desktopRefreshRate, 0, sizeof(m_desktopRefreshRate));
}

void DeviceInfo::Release()
{
    memset(&m_adapterDesc, 0, sizeof(m_adapterDesc));
    memset(&m_swapChainDesc, 0, sizeof(m_swapChainDesc));
    memset(&m_refreshRate, 0, sizeof(m_refreshRate));
    memset(&m_desktopRefreshRate, 0, sizeof(m_desktopRefreshRate));

    for (unsigned int b = 0; b < m_pBackbufferRTVs.size(); ++b)
    {
        SAFE_RELEASE(m_pBackbufferRTVs[b]);
    }
    if (m_pSwapChain)
    {
        m_pSwapChain->SetFullscreenState(FALSE, 0);
    }
    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pContext);
    SAFE_RELEASE(m_pDevice);
    SAFE_RELEASE(m_pOutput);
    SAFE_RELEASE(m_pAdapter);
    SAFE_RELEASE(m_pFactory);
}

static void SetupPreferredMonitorDimensions(HMONITOR hMonitor)
{
#if defined(WIN32) || defined(WIN64)
    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfo(hMonitor, &monitorInfo))
    {
        gcpRendD3D->m_prefMonX = monitorInfo.rcMonitor.left;
        gcpRendD3D->m_prefMonY = monitorInfo.rcMonitor.top;
        gcpRendD3D->m_prefMonWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        gcpRendD3D->m_prefMonHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
    }
#endif
}

static void SetupMonitorAndGetAdapterDesc(IDXGIOutput* pOutput, IDXGIAdapter1* pAdapter, DXGI_ADAPTER_DESC1& adapterDesc)
{
    DXGI_OUTPUT_DESC outputDesc;
    if (SUCCEEDED(pOutput->GetDesc(&outputDesc)))
    {
        SetupPreferredMonitorDimensions(outputDesc.Monitor);
    }
    pAdapter->GetDesc1(&adapterDesc);
}

static int GetDXGIAdapterOverride()
{
#if defined(WIN32) || defined(WIN64)
    ICVar* pCVar = gEnv->pConsole ? gEnv->pConsole->GetCVar("r_overrideDXGIAdapter") : 0;
    return pCVar ? pCVar->GetIVal() : -1;
#else
    return -1;
#endif
}

static void ProcessWindowMessages([[maybe_unused]] HWND hWnd)
{
#if defined(WIN32) || defined(WIN64)
    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
#endif
}

#if defined(OPENGL)
static int GetMultithreaded()
{
    ICVar* pCVar = gEnv->pConsole ? gEnv->pConsole->GetCVar("r_multithreaded") : 0;
    return pCVar ? pCVar->GetIVal() : -1;
}
#endif

#if !defined(_RELEASE)
bool GetForcedFeatureLevel(D3D_FEATURE_LEVEL* pForcedFeatureLevel)
{
    struct FeatureLevel
    {
        const char* m_szName;
        D3D_FEATURE_LEVEL m_eValue;
    };
    static FeatureLevel s_aLevels[] =
    {
        {"10.0", D3D_FEATURE_LEVEL_10_0},
        {"10.1", D3D_FEATURE_LEVEL_10_1},
        {"11.0", D3D_FEATURE_LEVEL_11_0},
        #if SUPPORTS_WINDOWS_10_SDK 
            {"11.1", D3D_FEATURE_LEVEL_11_1}
        #endif
    };

    const char* szForcedName(gcpRendD3D->CV_d3d11_forcedFeatureLevel->GetString());
    if (szForcedName == NULL || strlen(szForcedName) == 0)
    {
        return false;
    }

    FeatureLevel* pLevel;
    for (pLevel = s_aLevels; pLevel < s_aLevels + SIZEOF_ARRAY(s_aLevels); ++pLevel)
    {
        if (strcmp(pLevel->m_szName, szForcedName) == 0)
        {
            *pForcedFeatureLevel = pLevel->m_eValue;
            return true;
        }
    }

    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "Invalid value for d3d11_forcedFeatureLevel %s - using available feature level", szForcedName);
    return false;
}
#endif // !defined(_RELEASE)

bool DeviceInfo::CreateDevice(bool windowed, [[maybe_unused]] int width, [[maybe_unused]] int height, int backbufferWidth, int backbufferHeight, int zbpp, OnCreateDeviceCallback pCreateDeviceCallback, CreateWindowCallback pCreateWindowCallback)
{
#ifndef CRY_USE_METAL
    m_autoDepthStencilFmt = zbpp == 32 ? DXGI_FORMAT_R32G8X24_TYPELESS : DXGI_FORMAT_R24G8_TYPELESS;
#endif

#if defined(OPENGL)

    HWND hWnd = pCreateWindowCallback ? pCreateWindowCallback() : 0;
    if (!hWnd)
    {
        Release();
        return false;
    }

    const int r_overrideDXGIAdapter = GetDXGIAdapterOverride();
    const int r_multithreaded = GetMultithreaded();
    unsigned int nAdapterOrdinal = r_overrideDXGIAdapter >= 0 ? r_overrideDXGIAdapter : 0;
    InitSwapChain(m_swapChainDesc, backbufferWidth, backbufferHeight, hWnd, windowed);

    if (!SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**) &m_pFactory)) || !m_pFactory)
    {
        Release();
        return false;
    }

    while (m_pFactory->EnumAdapters1(nAdapterOrdinal, &m_pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        if (m_pAdapter)
        {
            m_driverType = D3D_DRIVER_TYPE_HARDWARE;
            m_creationFlags = 0;

            D3D_FEATURE_LEVEL* aFeatureLevels(NULL);
            unsigned int uNumFeatureLevels(0);
#if !defined(_RELEASE)
            D3D_FEATURE_LEVEL eForcedFeatureLevel(D3D_FEATURE_LEVEL_11_0);
            if (GetForcedFeatureLevel(&eForcedFeatureLevel))
            {
                aFeatureLevels = &eForcedFeatureLevel;
                uNumFeatureLevels = 1;
            }
#endif //!defined(_RELEASE)

            const D3D_DRIVER_TYPE driverType = m_driverType == D3D_DRIVER_TYPE_HARDWARE ? D3D_DRIVER_TYPE_UNKNOWN : m_driverType;
            HRESULT hr = D3D11CreateDeviceAndSwapChain(m_pAdapter, driverType, 0, m_creationFlags, aFeatureLevels, uNumFeatureLevels, D3D11_SDK_VERSION, &m_swapChainDesc, &m_pSwapChain, &m_pDevice, &m_featureLevel, &m_pContext);
            if (SUCCEEDED(hr) && m_pDevice && m_pSwapChain)
            {
#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
                const uint32 outputIdx = gRenDev->CV_r_overrideDXGIOutput > 0 ? gRenDev->CV_r_overrideDXGIOutput : 0;
                if (outputIdx)
                {
                    if (SUCCEEDED(m_pAdapter->EnumOutputs(outputIdx, &m_pOutput)) && m_pOutput)
                    {
                        SetupMonitorAndGetAdapterDesc(m_pOutput, m_pAdapter, m_adapterDesc);
                        break;
                    }

                    SAFE_RELEASE(m_pOutput);
                    CryLogAlways("Failed to resolve DXGI display for override index %d. Falling back to primary display.", outputIdx);
                }
#endif
                if (SUCCEEDED(m_pAdapter->EnumOutputs(0, &m_pOutput)) && m_pOutput)
                {
                    SetupMonitorAndGetAdapterDesc(m_pOutput, m_pAdapter, m_adapterDesc);
                    break;
                }
                else if (r_overrideDXGIAdapter >= 0)
                {
                    CryLogAlways("No display connected to DXGI adapter override %d. Adapter cannot be used for rendering.", r_overrideDXGIAdapter);
                }
            }

            SAFE_RELEASE(m_pOutput);
            SAFE_RELEASE(m_pContext);
            SAFE_RELEASE(m_pDevice);
            SAFE_RELEASE(m_pSwapChain);
            SAFE_RELEASE(m_pAdapter);
        }
    }

    if (!m_pDevice || !m_pSwapChain)
    {
        Release();
        return false;
    }

    m_pFactory->MakeWindowAssociation(m_swapChainDesc.OutputWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

    if (pCreateDeviceCallback)
    {
        pCreateDeviceCallback(m_pDevice);
    }

#if !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    if (r_multithreaded)
    {
        DXGLReserveContext(m_pDevice);
    }
    DXGLBindDeviceContext(m_pContext, !r_multithreaded);
#endif //!DXGL_FULL_EMULATION
    bool bSuccess = CreateViews();
    if (!bSuccess)
    {
        Release();
    }

    return IsOk();
#elif defined(WIN32) || defined(WIN64)
    typedef HRESULT (WINAPI * FP_CreateDXGIFactory1)(REFIID, void**);

    FP_CreateDXGIFactory1 pCDXGIF =
#if defined(CRY_USE_DX12)
        (FP_CreateDXGIFactory1) DX12CreateDXGIFactory1;
#else
        (FP_CreateDXGIFactory1) GetProcAddress(LoadLibraryA("dxgi.dll"), "CreateDXGIFactory1");
#endif

    IDXGIAdapter1* pAdapter = nullptr;
    IDXGIOutput* pOutput = nullptr;
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;

    if (pCDXGIF && SUCCEEDED(pCDXGIF(__uuidof(IDXGIFactory1), (void**) &m_pFactory)) && m_pFactory)
    {
        typedef HRESULT (WINAPI * CreateDeviceCallback)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, CONST D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

        CreateDeviceCallback createDeviceCallback =
#if defined(CRY_USE_DX12)
            (CreateDeviceCallback)DX12CreateDevice;
#else
            (CreateDeviceCallback)GetProcAddress(LoadLibraryA("d3d11.dll"), "D3D11CreateDevice");
#endif

        if (createDeviceCallback)
        {
            const int r_overrideDXGIAdapter = GetDXGIAdapterOverride();
            unsigned int nAdapterOrdinal = r_overrideDXGIAdapter >= 0 ? r_overrideDXGIAdapter : 0;

            while (m_pFactory->EnumAdapters1(nAdapterOrdinal, &pAdapter) != DXGI_ERROR_NOT_FOUND)
            {
                if (pAdapter)
                {
                    // Promote interfaces to the required level
                    pAdapter->QueryInterface(__uuidof(DXGIAdapter), (void**)&m_pAdapter);

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
                    const unsigned int debugRTFlag = gcpRendD3D->CV_d3d11_debugruntime ? D3D11_CREATE_DEVICE_DEBUG : 0;
#else
                    const unsigned int debugRTFlag = 0;
#endif
                    m_driverType = D3D_DRIVER_TYPE_HARDWARE;
                    m_creationFlags = debugRTFlag;
#if defined(WIN32) || defined(WIN64)
                    // CRY DX12 DISABLED TEMPORARILY                    if (gcpRendD3D->CV_d3d11_preventDriverThreading)
                    //                      m_creationFlags |= D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
#endif

                    D3D_FEATURE_LEVEL arrFeatureLevels[] = {
                        #if SUPPORTS_WINDOWS_10_SDK 
                            D3D_FEATURE_LEVEL_11_1, 
                        #endif
                        D3D_FEATURE_LEVEL_11_0
                    };
                    D3D_FEATURE_LEVEL* pFeatureLevels(arrFeatureLevels);
                    unsigned int uNumFeatureLevels(sizeof(arrFeatureLevels) / sizeof(D3D_FEATURE_LEVEL));
#if !defined(_RELEASE)
                    D3D_FEATURE_LEVEL eForcedFeatureLevel;
                    if (GetForcedFeatureLevel(&eForcedFeatureLevel))
                    {
                        pFeatureLevels = &eForcedFeatureLevel;
                        uNumFeatureLevels = 1;
                    }
#endif //!defined(_RELEASE)

                    DXGI_ADAPTER_DESC1 adapterDesc1;
                    if (SUCCEEDED(m_pAdapter->GetDesc1(&adapterDesc1)))
                    {
                        // We need to know if this is a software adapter (e.g., WARP) which may not have any outputs
                        m_adapterFlag = static_cast<DXGI_ADAPTER_FLAG>(adapterDesc1.Flags);
                    }

                    const D3D_DRIVER_TYPE driverType = m_driverType == D3D_DRIVER_TYPE_HARDWARE ? D3D_DRIVER_TYPE_UNKNOWN : m_driverType;
                    HRESULT hr = createDeviceCallback(pAdapter, driverType, 0, m_creationFlags, pFeatureLevels, uNumFeatureLevels, D3D11_SDK_VERSION, &pDevice, &m_featureLevel, &pContext);
                    if (SUCCEEDED(hr) && pDevice)
                    {
                        // Promote interfaces to the required level
                        pDevice->QueryInterface(__uuidof(D3DDevice), (void**)&m_pDevice);
                        pContext->QueryInterface(__uuidof(D3DDeviceContext), (void**)&m_pContext);

                        {
                            DXGIDevice* pDXGIDevice = 0;
                            if (SUCCEEDED(pDevice->QueryInterface(__uuidof(DXGIDevice), (void**) &pDXGIDevice)) && pDXGIDevice)
                            {
                                // SetMaximumFrameLatency as 3 in editor mode to avoid waiting GPU during Present.
                                // Because there might be multiple Presents in one update loop (main render present, material editor present...), 
                                // setting it as 1 is not enough in this case, during the same update loop, the 2nd present can wait significant time for 1st presented GPU draw.
                                pDXGIDevice->SetMaximumFrameLatency(gcpRendD3D->IsEditorMode() ? 3 : 1);
                            }
                            SAFE_RELEASE(pDXGIDevice);
                        }

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
                        m_outputIndex = gRenDev->CV_r_overrideDXGIOutput > 0 ? gRenDev->CV_r_overrideDXGIOutput : 0;
                        if (m_outputIndex)
                        {
                            if (SUCCEEDED(pAdapter->EnumOutputs(m_outputIndex, &pOutput)) && pOutput)
                            {
                                // Promote interfaces to the required level
                                pOutput->QueryInterface(__uuidof(DXGIOutput), (void**)&m_pOutput);
                                SetupMonitorAndGetAdapterDesc(m_pOutput, m_pAdapter, m_adapterDesc);
                                break;
                            }

                            SAFE_RELEASE(pOutput);
                            CryLogAlways("Failed to resolve DXGI display for override index %d. Falling back to primary display.", m_outputIndex);
                            m_outputIndex = 0;
                        }
#endif
                        if (SUCCEEDED(pAdapter->EnumOutputs(0, &pOutput)) && pOutput)
                        {
                            // Promote interfaces to the required level
                            pOutput->QueryInterface(__uuidof(DXGIOutput), (void**)&m_pOutput);
                            SetupMonitorAndGetAdapterDesc(m_pOutput, m_pAdapter, m_adapterDesc);
                            break;
                        }
                        else if (m_adapterFlag == DXGI_ADAPTER_FLAG_SOFTWARE || m_driverType == D3D_DRIVER_TYPE_WARP)
                        {
                            break;
                        }
                        else if (r_overrideDXGIAdapter >= 0)
                        {
                            CryLogAlways("No display connected to DXGI adapter override %d. Adapter cannot be used for rendering.", r_overrideDXGIAdapter);
                        }
                    }

                    // Decrement QueryInterface() increment
                    SAFE_RELEASE(m_pOutput);
                    SAFE_RELEASE(m_pContext);
                    SAFE_RELEASE(m_pDevice);
                    SAFE_RELEASE(m_pAdapter);

                    // Decrement Create() increment
                    SAFE_RELEASE(pOutput);
                    SAFE_RELEASE(pContext);
                    SAFE_RELEASE(pDevice);
                    SAFE_RELEASE(pAdapter);
                }

                if (r_overrideDXGIAdapter >= 0)
                {
                    break;
                }

                ++nAdapterOrdinal;
            }
        }
    }

    if (!m_pFactory || !m_pAdapter || !m_pDevice || !m_pContext)
    {
        Release();
        return false;
    }

    if (!m_pOutput && m_adapterFlag != DXGI_ADAPTER_FLAG_SOFTWARE && m_driverType != D3D_DRIVER_TYPE_WARP)
    {
        Release();
        return false;
    }

    // Decrement Create() increment
    SAFE_RELEASE(pOutput);
    SAFE_RELEASE(pContext);
    SAFE_RELEASE(pDevice);
    SAFE_RELEASE(pAdapter);

    // Get SDK level D3D device pointer
#ifdef CRY_INTEGRATE_DX12
        IUnknown* d3dDevice = static_cast<CCryDX12Device*>(m_pDevice)->GetD3D12Device();
#else
        IUnknown* d3dDevice = m_pDevice;
#endif

#if defined(WIN32) || defined(WIN64)
    {
        DXGI_MODE_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.Width = gcpRendD3D->m_prefMonWidth;
        desc.Height = gcpRendD3D->m_prefMonHeight;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        DXGI_MODE_DESC match;
        if (m_pOutput && SUCCEEDED(m_pOutput->FindClosestMatchingMode(&desc, &match, d3dDevice)))
        {
            m_desktopRefreshRate = match.RefreshRate;
        }
    }
#endif

    HWND hWnd = pCreateWindowCallback ? pCreateWindowCallback() : 0;
    if (!hWnd)
    {
        Release();
        return false;
    }

    ProcessWindowMessages(hWnd);

    {
        InitSwapChain(m_swapChainDesc, backbufferWidth, backbufferHeight, hWnd, windowed);

        if (!windowed)
        {
            DXGI_MODE_DESC match;
            if (m_pOutput && SUCCEEDED(m_pOutput->FindClosestMatchingMode(&m_swapChainDesc.BufferDesc, &match, d3dDevice)))
            {
                m_swapChainDesc.BufferDesc = match;
            }
        }

        m_refreshRate = !windowed ? m_swapChainDesc.BufferDesc.RefreshRate : m_desktopRefreshRate;

        IDXGISwapChain* pSwapChain;
        HRESULT hr = m_pFactory->CreateSwapChain(m_pDevice, &m_swapChainDesc, &pSwapChain);
        if (FAILED(hr) || !pSwapChain)
        {
            Release();
            return false;
        }

        // Promote interfaces to the required level
        hr = pSwapChain->QueryInterface(__uuidof(DXGISwapChain), (void**)&m_pSwapChain);
        if (FAILED(hr) || !m_pSwapChain)
        {
            Release();
            return false;
        }
#ifdef CRY_INTEGRATE_DX12
        m_pSwapChain->SetMaximumFrameLatency(1);
        m_frameLatencyWaitableObject = m_pSwapChain->GetFrameLatencyWaitableObject();
#endif
        // Decrement Create() increment
        SAFE_RELEASE(pSwapChain);
    }

    {
        m_pFactory->MakeWindowAssociation(m_swapChainDesc.OutputWindow, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

        if (pCreateDeviceCallback)
        {
            pCreateDeviceCallback(m_pDevice);
        }
    }

    if (!CreateViews())
    {
        Release();
    }

    ProcessWindowMessages(hWnd);

    return IsOk();
#else
    #pragma message("DeviceInfo::CreateDevice not implemented on this platform")
    return false;
#endif
}

bool DeviceInfo::CreateViews()
{
    DXGI_SWAP_CHAIN_DESC scDesc;

    m_pSwapChain->GetDesc(&scDesc);

    AZ::u32 bufferCount = scDesc.BufferCount;
#if !defined(CRY_USE_DX12)
    bufferCount = 1;
#endif

    m_pBackbufferRTVs.resize(bufferCount);
    for (unsigned int b = 0; b < bufferCount; ++b)
    {
        D3DTexture* pBackBuffer = 0;
        HRESULT hr = m_pSwapChain->GetBuffer(b, __uuidof(D3DTexture), (void**) &pBackBuffer);

        if (SUCCEEDED(hr) && pBackBuffer)
        {
            {
                assert(!m_pBackbufferRTVs[b]);
                hr = m_pDevice->CreateRenderTargetView(pBackBuffer, 0, &m_pBackbufferRTVs[b]);
            }

#if !defined(RELEASE) && defined(WIN64)
            char str[32] = "";
            azsprintf(str, "Swap-Chain back buffer %d", b);
            pBackBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(str), str);
#endif
        }

        SAFE_RELEASE(pBackBuffer);
    }

    m_pCurrentBackBufferRTVIndex = CD3D9Renderer::GetCurrentBackBufferIndex(m_pSwapChain);

    return true;
}

void DeviceInfo::SnapSettings()
{
    if (m_swapChainDesc.Windowed)
    {
        m_swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
        m_swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        m_swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        m_swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        m_refreshRate = m_desktopRefreshRate;
    }
    else
    {
        DXGI_MODE_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.Width = m_swapChainDesc.BufferDesc.Width;
        desc.Height = m_swapChainDesc.BufferDesc.Height;
        desc.Format = m_swapChainDesc.BufferDesc.Format;

        DXGI_MODE_DESC match;
#ifdef CRY_INTEGRATE_DX12
        IUnknown* d3dDevice = static_cast<CCryDX12Device*>(m_pDevice)->GetD3D12Device();
#else
        IUnknown* d3dDevice = m_pDevice;
#endif
        if (m_pOutput && SUCCEEDED(m_pOutput->FindClosestMatchingMode(&desc, &match, d3dDevice)))
        {
            m_swapChainDesc.BufferDesc = match;

            m_refreshRate = match.RefreshRate;
        }
    }
}

void DeviceInfo::ResizeDXGIBuffers()
{
    for (unsigned int b = 0; b < m_pBackbufferRTVs.size(); ++b)
    {
        SAFE_RELEASE(m_pBackbufferRTVs[b]);
    }


    UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#ifdef CRY_INTEGRATE_DX12
    flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
#endif

    HRESULT hr = m_pSwapChain->ResizeBuffers(0, m_swapChainDesc.BufferDesc.Width, m_swapChainDesc.BufferDesc.Height, m_swapChainDesc.BufferDesc.Format, flags);

    CreateViews();
}

#if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)
void DeviceInfo::OnActivate(UINT_PTR wParam, UINT_PTR lParam)
{
    const bool activate = LOWORD(wParam) != 0;
    if (m_activatedMT != activate)
    {
        if (gcpRendD3D->IsFullscreen())
        {
            HWND hWnd = (HWND) gcpRendD3D->GetHWND();
            ShowWindow(hWnd, activate ? SW_RESTORE : SW_MINIMIZE);
        }

        m_activatedMT = activate;
    }

    PushSystemEvent(ESYSTEM_EVENT_ACTIVATE, wParam, lParam);
}

void DeviceInfo::PushSystemEvent(ESystemEvent event, UINT_PTR wParam, UINT_PTR lParam)
{
#if !defined(_RELEASE) && !defined(STRIP_RENDER_THREAD)
    if (gcpRendD3D->m_pRT && !gcpRendD3D->m_pRT->IsMainThread())
    {
        __debugbreak();
    }
#endif
    CryAutoCriticalSection lock(m_msgQueueLock);
    m_msgQueue.push_back(DeviceInfoInternal::MsgQueueItem(event, wParam, lParam));
}

void DeviceInfo::ProcessSystemEventQueue()
{
#if !defined(_RELEASE)
    if (gcpRendD3D->m_pRT && !gcpRendD3D->m_pRT->IsRenderThread())
    {
        __debugbreak();
    }
#endif

    m_msgQueueLock.Lock();
    DeviceInfoInternal::MsgQueue localQueue;
    localQueue.swap(m_msgQueue);
    m_msgQueueLock.Unlock();

    for (size_t i = 0; i < localQueue.size(); ++i)
    {
        const DeviceInfoInternal::MsgQueueItem& item = localQueue[i];
        ProcessSystemEvent(item.event, item.wParam, item.lParam);
    }
}

void DeviceInfo::ProcessSystemEvent(ESystemEvent event, UINT_PTR wParam, [[maybe_unused]] UINT_PTR lParam)
{
#if !defined(_RELEASE)
    if (gcpRendD3D->m_pRT && !gcpRendD3D->m_pRT->IsRenderThread())
    {
        __debugbreak();
    }
#endif

    switch (event)
    {
    case ESYSTEM_EVENT_ACTIVATE:
    {
#if defined(WIN32)
        // disable floating point exceptions due to driver bug with floating point exceptions
        SCOPED_DISABLE_FLOAT_EXCEPTIONS;
#endif
        const bool activate = LOWORD(wParam) != 0;
        if (m_activated != activate)
        {
            HWND hWnd = (HWND) gcpRendD3D->GetHWND();

            const bool isFullscreen = gcpRendD3D->IsFullscreen();
            if (isFullscreen && !activate)
            {
                gcpRendD3D->GetS3DRend().ReleaseBuffers();
                m_pSwapChain->SetFullscreenState(FALSE, 0);
                ResizeDXGIBuffers();
                gcpRendD3D->OnD3D11PostCreateDevice(m_pDevice);
            }
            else if (isFullscreen && activate)
            {
                gcpRendD3D->GetS3DRend().ReleaseBuffers();
                m_pSwapChain->SetFullscreenState(TRUE, 0);
                ResizeDXGIBuffers();
                gcpRendD3D->GetS3DRend().OnResolutionChanged();
                gcpRendD3D->OnD3D11PostCreateDevice(m_pDevice);
            }

            m_activated = activate;
        }
    }
    break;

    default:
        assert(0);
        break;
    }
}
#endif // #if defined(SUPPORT_DEVICE_INFO_MSG_PROCESSING)

#if defined(WIN32) || defined(WIN64)
void DeviceInfo::EnforceFullscreenPreemption()
{
    if (gRenDev->CV_r_FullscreenPreemption && gcpRendD3D->IsFullscreen())
    {
        HRESULT hr = m_pSwapChain->Present(0, DXGI_PRESENT_TEST);
        if (hr == DXGI_STATUS_OCCLUDED)
        {
            HWND hWnd = (HWND) gcpRendD3D->GetHWND();
            if (m_activated)
            {
                BringWindowToTop(hWnd);
            }
        }
    }
}

#ifdef CRY_INTEGRATE_DX12
void DeviceInfo::WaitForGPUFrames()
{
    FUNCTION_PROFILER_RENDER_FLAT
    WaitForSingleObjectEx(
        m_frameLatencyWaitableObject,
        1000, // 1 second timeout (shouldn't ever occur)
        true
    );
}
#endif

#endif // #if defined(WIN32) || defined(WIN64)

#endif // #if defined(SUPPORT_DEVICE_INFO)
