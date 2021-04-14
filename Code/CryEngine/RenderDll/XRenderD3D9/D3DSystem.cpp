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
#include "UnicodeFunctions.h"
#include "DriverD3D.h"
#include "WindowsUtils.h"

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DSYSTEM_CPP_SECTION_1 1
#define D3DSYSTEM_CPP_SECTION_2 2
#define D3DSYSTEM_CPP_SECTION_3 3
#define D3DSYSTEM_CPP_SECTION_4 4
#define D3DSYSTEM_CPP_SECTION_5 5
#define D3DSYSTEM_CPP_SECTION_6 6
#define D3DSYSTEM_CPP_SECTION_7 7
#define D3DSYSTEM_CPP_SECTION_8 8
#define D3DSYSTEM_CPP_SECTION_9 9
#define D3DSYSTEM_CPP_SECTION_10 10
#define D3DSYSTEM_CPP_SECTION_11 11
#define D3DSYSTEM_CPP_SECTION_12 12
#define D3DSYSTEM_CPP_SECTION_13 13
#define D3DSYSTEM_CPP_SECTION_14 14
#define D3DSYSTEM_CPP_SECTION_15 15
#define D3DSYSTEM_CPP_SECTION_16 16
#define D3DSYSTEM_CPP_SECTION_17 17
#define D3DSYSTEM_CPP_SECTION_18 18
#define D3DSYSTEM_CPP_SECTION_19 19
#define D3DSYSTEM_CPP_SECTION_20 20
#define D3DSYSTEM_CPP_SECTION_21 21
#define D3DSYSTEM_CPP_SECTION_22 22
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif


#include "D3DStereo.h"
#include "D3DPostProcess.h"
#include "NullD3D11Device.h"

#if defined(WIN32)

// CryTek undefs some defines from WinBase.h...so let's redefine them here
  #ifdef UNICODE
    #define RegisterClass  RegisterClassW
    #define LoadLibrary  LoadLibraryW
  #else
    #define RegisterClass  RegisterClassA
    #define LoadLibrary  LoadLibraryA
  #endif // !UNICODE
#endif // defined(WIN32)

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif

#include "../Common/RenderCapabilities.h"
#include <AzCore/Utils/Utils.h>

#ifdef WIN32
// Count monitors helper
static BOOL CALLBACK CountConnectedMonitors([[maybe_unused]] HMONITOR hMonitor, [[maybe_unused]] HDC hDC, [[maybe_unused]] LPRECT pRect, LPARAM opaque)
{
    uint* count = reinterpret_cast<uint*>(opaque);
    (*count)++;
    return TRUE;
}

#endif

void CD3D9Renderer::DisplaySplash()
{
#ifdef WIN32
    if (IsEditorMode())
    {
        return;
    }

    HBITMAP hImage = (HBITMAP)LoadImage(GetModuleHandle(0), "splash.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    if (hImage != INVALID_HANDLE_VALUE)
    {
        RECT rect;
        HDC hDC = GetDC(m_hWnd);
        HDC hDCBitmap = CreateCompatibleDC(hDC);
        BITMAP bm;

        GetClientRect(m_hWnd, &rect);
        GetObjectA(hImage, sizeof(bm), &bm);
        SelectObject(hDCBitmap, hImage);



        //    BitBlt(hDC, x, y, bm.bmWidth, bm.bmHeight, hDCBitmap, 0, 0, SRCCOPY);

        RECT Rect;
        GetWindowRect(m_hWnd, &Rect);
        StretchBlt(hDC, 0, 0, Rect.right - Rect.left, Rect.bottom - Rect.top,  hDCBitmap, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

        DeleteObject(hImage);
        DeleteDC(hDCBitmap);
    }
#endif
}

//=====================================================================================

bool CD3D9Renderer::SetCurrentContext(WIN_HWND hWnd)
{
    uint32 i;

    for (i = 0; i < m_RContexts.Num(); i++)
    {
        if (m_RContexts[i]->m_hWnd == hWnd)
        {
            break;
        }
    }
    if (i == m_RContexts.Num())
    {
        return false;
    }

    if (m_CurrContext == m_RContexts[i])
    {
        return true;
    }

    m_CurrContext = m_RContexts[i];

    CHWShader::s_pCurVS = NULL;
    CHWShader::s_pCurPS = NULL;

    return true;
}

bool CD3D9Renderer::CreateContext(WIN_HWND hWnd, [[maybe_unused]] bool bAllowMSAA, int SSX, int SSY)
{
    LOADING_TIME_PROFILE_SECTION;
    uint32 i;

    for (i = 0; i < m_RContexts.Num(); i++)
    {
        if (m_RContexts[i]->m_hWnd == hWnd)
        {
            break;
        }
    }
    if (i != m_RContexts.Num())
    {
        return true;
    }
    SD3DContext* pContext = new SD3DContext;
    pContext->m_hWnd = (HWND)hWnd;
    pContext->m_X = 0;
    pContext->m_Y = 0;
    pContext->m_Width  = m_width;
    pContext->m_Height = m_height;
    pContext->m_pSwapChain  = 0;
    pContext->m_pBackBuffer = 0;
    pContext->m_nViewportWidth  = aznumeric_cast<int>(aznumeric_cast<float>(m_width)  / (m_CurrContext ? m_CurrContext->m_fPixelScaleX : 1.0f));
    pContext->m_nViewportHeight = aznumeric_cast<int>(aznumeric_cast<float>(m_height) / (m_CurrContext ? m_CurrContext->m_fPixelScaleY : 1.0f));
    pContext->m_fPixelScaleX = aznumeric_cast<float>(std::max(1, SSX));
    pContext->m_fPixelScaleY = aznumeric_cast<float>(std::max(1, SSY));
    pContext->m_bMainViewport = !gEnv->IsEditor();
    m_CurrContext = pContext;
    m_RContexts.AddElem(pContext);

    return true;
}

bool CD3D9Renderer::DeleteContext(WIN_HWND hWnd)
{
    uint32 i, j;

    for (i = 0; i < m_RContexts.Num(); i++)
    {
        if (m_RContexts[i]->m_hWnd == hWnd)
        {
            break;
        }
    }
    if (i == m_RContexts.Num())
    {
        return false;
    }
    if (m_CurrContext == m_RContexts[i])
    {
        for (j = 0; j < m_RContexts.Num(); j++)
        {
            if (m_RContexts[j]->m_hWnd != hWnd)
            {
                m_CurrContext = m_RContexts[j];
                break;
            }
        }
        if (j == m_RContexts.Num())
        {
            m_CurrContext = NULL;
        }

        if (!m_CurrContext)
        {
            m_width = 0;
            m_height = 0;
        }
        else if (m_CurrContext->m_Width != m_width || m_CurrContext->m_Height != m_height)
        {
            m_width = m_CurrContext->m_Width;
            m_height = m_CurrContext->m_Height;
        }
    }
    for (unsigned int b = 0; b < m_RContexts[i]->m_pBackBuffers.size(); ++b)
    {
        SAFE_RELEASE(m_RContexts[i]->m_pBackBuffers[b]);
    }
    SAFE_RELEASE(m_RContexts[i]->m_pSwapChain);
    delete m_RContexts[i];
    m_RContexts.Remove(i, 1);

    return true;
}

void CD3D9Renderer::MakeMainContextActive()
{
    if (m_RContexts.empty() || m_CurrContext == m_RContexts[0])
    {
        return;
    }

    m_CurrContext = m_RContexts[0];

    CHWShader::s_pCurVS = NULL;
    CHWShader::s_pCurPS = NULL;
}

bool CD3D9Renderer::CreateMSAADepthBuffer()
{
    HRESULT hr = S_OK;
    if (CV_r_msaa)
    {
        if (m_RP.m_MSAAData.Type != CV_r_msaa_samples || m_RP.m_MSAAData.Quality != CV_r_msaa_quality)
        {
            SAFE_RELEASE(m_RP.m_MSAAData.m_pZBuffer);
            SAFE_RELEASE(m_RP.m_MSAAData.m_pDepthTex);
        }
        m_RP.m_MSAAData.Type = CV_r_msaa_samples;
        m_RP.m_MSAAData.Quality = CV_r_msaa_quality;
        if (m_RP.m_MSAAData.Type > 1 && !m_RP.m_MSAAData.m_pZBuffer)
        {
            // Create depth stencil texture
            D3D11_TEXTURE2D_DESC descDepth;
            ZeroStruct(descDepth);
            descDepth.Width = m_width;
            descDepth.Height = m_height;
            descDepth.MipLevels = 1;
            descDepth.ArraySize = 1;
            descDepth.Format = m_ZFormat;
            descDepth.SampleDesc.Count = m_RP.m_MSAAData.Type;
            descDepth.SampleDesc.Quality = m_RP.m_MSAAData.Quality;
            descDepth.Usage = D3D11_USAGE_DEFAULT;
            descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // bind-able as shader resource view
            descDepth.CPUAccessFlags = 0;
            descDepth.MiscFlags = 0;

            const float clearDepth = CRenderer::CV_r_ReverseDepth ? 0.f : 1.f;
            const uint clearStencil = 1;
            const FLOAT clearValues[4] = { clearDepth, FLOAT(clearStencil) };

            hr = m_DevMan.CreateD3D11Texture2D(&descDepth, clearValues, NULL, &m_RP.m_MSAAData.m_pDepthTex, "MSAADepthBuffer");
            if (FAILED(hr))
            {
                return false;
            }

            m_DepthBufferOrigMSAA.pTex = nullptr;
            m_DepthBufferOrigMSAA.pTarget = m_RP.m_MSAAData.m_pDepthTex;
            m_DepthBufferOrigMSAA.pSurf = m_RP.m_MSAAData.m_pZBuffer;

            // Create the depth stencil view
            D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
            ZeroStruct(descDSV);
            descDSV.Format =  CTexture::ConvertToDepthStencilFmt(descDepth.Format);
            descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
            hr = GetDevice().CreateDepthStencilView(m_RP.m_MSAAData.m_pDepthTex, &descDSV, &m_RP.m_MSAAData.m_pZBuffer);
            if (FAILED(hr))
            {
                return false;
            }
            m_DepthBufferOrigMSAA.pSurf = m_RP.m_MSAAData.m_pZBuffer;
            m_RP.m_MSAAData.m_pZBuffer->AddRef();
        }
    }
    else
    {
        m_RP.m_MSAAData.Type = 0;
        m_RP.m_MSAAData.Quality = 0;

        SAFE_RELEASE(m_RP.m_MSAAData.m_pZBuffer);
        SAFE_RELEASE(m_RP.m_MSAAData.m_pDepthTex);
        
        SAFE_RELEASE(m_DepthBufferOrigMSAA.pSurf);
        m_DepthBufferOrigMSAA.pTex = nullptr;
        m_DepthBufferOrigMSAA.pSurf = m_pZBuffer;
        m_DepthBufferOrigMSAA.pTarget = m_pZTexture;
        m_pZBuffer->AddRef();
    }
    return (hr == S_OK);
}

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
static void UserOverrideDXGIOutputFS(DeviceInfo& devInfo, int outputIndex, int defaultX, int defaultY, int& outputX, int& outputY)
{
    outputX = defaultX;
    outputY = defaultY;

    // This is not an ideal solution. Just for development or careful use.
    // The FS output override might be incompatible with output originally set up in device info.
    // As such selected resolutions might not be directly supported but currently won't fall back properly.
#   if (defined(WIN32) || defined(WIN64))
    if (outputIndex > 0)
    {
        bool success = false;

        IDXGIOutput* pOutput = 0;
        if (SUCCEEDED(devInfo.Adapter()->EnumOutputs(outputIndex, &pOutput)) && pOutput)
        {
            DXGI_OUTPUT_DESC outputDesc;
            if (SUCCEEDED(pOutput->GetDesc(&outputDesc)))
            {
                MONITORINFO monitorInfo;
                monitorInfo.cbSize = sizeof(monitorInfo);
                if (GetMonitorInfo(outputDesc.Monitor, &monitorInfo))
                {
                    outputX = monitorInfo.rcMonitor.left;
                    outputY = monitorInfo.rcMonitor.top;
                    success = true;
                }
            }
        }
        SAFE_RELEASE(pOutput);

        if (!success)
        {
            CryLogAlways("Failed to resolve DXGI display for override index %d. Falling back to preferred or primary display.", outputIndex);
        }
    }
#   endif
}
#endif

bool CD3D9Renderer::ChangeResolution(int nNewWidth, int nNewHeight, int nNewColDepth, [[maybe_unused]] int nNewRefreshHZ, bool bFullScreen, bool bForceReset)
{
    if (m_bDeviceLost)
    {
        return true;
    }

#if !defined(_RELEASE) && (defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX))
    if (m_pRT && !m_pRT->IsRenderThread())
    {
        __debugbreak();
    }
#endif

    iLog->Log("Changing resolution...");

    const int nPrevWidth = m_width;
    const int nPrevHeight = m_height;
    const int nPrevColorDepth = m_cbpp;
    const bool bPrevFullScreen = m_bFullScreen;
    if (nNewColDepth < 24)
    {
        nNewColDepth = 16;
    }
    else
    {
        nNewColDepth = 32;
    }
    bool bNeedReset = bForceReset || nNewColDepth != nPrevColorDepth || bFullScreen != bPrevFullScreen || nNewWidth != nPrevWidth || nNewHeight != nPrevHeight;
#if !defined(SUPPORT_DEVICE_INFO)
    bNeedReset |= m_VSync != CV_r_vsync;
#endif

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
    bNeedReset |= m_overrideRefreshRate != CV_r_overrideRefreshRate || m_overrideScanlineOrder != CV_r_overrideScanlineOrder;
#endif

    GetS3DRend().ReleaseBuffers();
    DeleteContext(m_hWnd);

    // Save the new dimensions
    m_width  = nNewWidth;
    m_height = nNewHeight;
    m_cbpp   = nNewColDepth;
    m_bFullScreen = bFullScreen;
#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
    m_overrideRefreshRate = CV_r_overrideRefreshRate;
    m_overrideScanlineOrder = CV_r_overrideScanlineOrder;
#endif
    if (!IsEditorMode())
    {
        m_VSync = CV_r_vsync;
    }
    else
    {
        m_VSync = 0;
    }
#if defined(SUPPORT_DEVICE_INFO)
    m_devInfo.SyncInterval() = m_VSync ? 1 : 0;
#endif

    if (bFullScreen && nNewColDepth == 16)
    {
        m_zbpp = 16;
        m_sbpp = 0;
    }

    RestoreGamma();

    bool bFullscreenWindow = false;
#if defined(WIN32) || defined(WIN64)
    bFullscreenWindow = CV_r_FullscreenWindow && CV_r_FullscreenWindow->GetIVal() != 0;
#endif

    if (IsEditorMode() && !bForceReset)
    {
        nNewWidth = m_deskwidth;
        nNewHeight = m_deskheight;
    }
    if (bNeedReset)
    {
#if defined(SUPPORT_DEVICE_INFO)
#if defined(WIN32)
        // disable floating point exceptions due to driver bug when switching to fullscreen
        SCOPED_DISABLE_FLOAT_EXCEPTIONS;
#endif
        m_devInfo.SwapChainDesc().Windowed = !bFullScreen;
        m_devInfo.SwapChainDesc().BufferDesc.Width = m_backbufferWidth;
        m_devInfo.SwapChainDesc().BufferDesc.Height = m_backbufferHeight;

        m_devInfo.SnapSettings();

        int swapChainWidth  = m_devInfo.SwapChainDesc().BufferDesc.Width;
        int swapChainHeight = m_devInfo.SwapChainDesc().BufferDesc.Height;
        if (m_backbufferWidth != swapChainWidth || m_backbufferHeight != swapChainHeight)
        {
            if (m_nativeWidth == m_backbufferWidth)
            {
                if (m_width == m_nativeWidth)
                {
                    m_width = swapChainWidth;
                    if (m_CVWidth)
                    {
                        m_CVWidth->Set(swapChainWidth);
                    }
                }
                m_nativeWidth = swapChainWidth;
            }
            m_backbufferWidth  = swapChainWidth;

            if (m_nativeHeight == m_backbufferHeight)
            {
                if (m_height == m_nativeHeight)
                {
                    m_height = swapChainHeight;
                    if (m_CVHeight)
                    {
                        m_CVHeight->Set(swapChainHeight);
                    }
                }
                m_nativeHeight = swapChainHeight;
            }
            m_backbufferHeight = swapChainHeight;
        }

        ID3D11DepthStencilView* pDSV = 0;
        ID3D11RenderTargetView* pRTVs[8] = {0};
        GetDeviceContext().OMSetRenderTargets(8, pRTVs, pDSV);
        m_DepthBufferOrig.Release();
        m_DepthBufferOrigMSAA.Release();
        m_DepthBufferNative.Release();

        AdjustWindowForChange();


#   if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
        UserOverrideDisplayProperties(m_devInfo.SwapChainDesc().BufferDesc);
#   endif
        m_pSwapChain->SetFullscreenState(bFullScreen, 0);
        m_pSwapChain->ResizeTarget(&m_devInfo.SwapChainDesc().BufferDesc);
        m_devInfo.ResizeDXGIBuffers();

        OnD3D11PostCreateDevice(m_devInfo.Device());
#endif
        m_FullResRect.right = m_width;
        m_FullResRect.bottom = m_height;

#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE) || defined(CREATE_DEVICE_ON_MAIN_THREAD)
        m_pRT->RC_SetViewport(0, 0, m_width, m_height);
#else
        RT_SetViewport(0, 0, m_width, m_height);
#endif
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
        m_MainViewport.nX = 0;
        m_MainViewport.nY = 0;
        m_MainViewport.nWidth = m_width;
        m_MainViewport.nHeight = m_height;
        m_MainRTViewport.nX = 0;
        m_MainRTViewport.nY = 0;
        m_MainRTViewport.nWidth = m_width;
        m_MainRTViewport.nHeight = m_height;
    }

    AdjustWindowForChange();

    GetS3DRend().OnResolutionChanged();

#ifdef WIN32
    SetWindowText(m_hWnd, m_WinTitle);
    iLog->Log("  Window resolution: %dx%dx%d (%s)", m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, nNewColDepth, bFullScreen ? "Fullscreen" : "Windowed");
    iLog->Log("  Render resolution: %dx%d)", m_width, m_height);
#endif

    CreateMSAADepthBuffer();

    CreateContext(m_hWnd, CV_r_msaa != 0);

    ICryFont* pCryFont = gEnv->pCryFont;
    if (pCryFont)
    {
        pCryFont->GetFont("default");
    }

    PostDeviceReset();

    return true;
}

void CD3D9Renderer::PostDeviceReset()
{
    m_bDeviceLost = 0;
    if (m_bFullScreen)
    {
        SetGamma(CV_r_gamma + m_fDeltaGamma, CV_r_brightness, CV_r_contrast, true);
    }
    FX_ResetPipe();
    CHWShader::s_pCurVS = NULL;
    CHWShader::s_pCurPS = NULL;

    for (int i = 0; i < MAX_TMU; i++)
    {
        CTexture::s_TexStages[i].m_DevTexture = NULL;
    }
    m_nFrameReset++;
}


//-----------------------------------------------------------------------------
// Name: CD3D9Renderer::AdjustWindowForChange()
// Desc: Prepare the window for a possible change between windowed mode and
//       fullscreen mode.  This function is virtual and thus can be overridden
//       to provide different behavior, such as switching to an entirely
//       different window for fullscreen mode (as in the MFC sample apps).
//-----------------------------------------------------------------------------
HRESULT CD3D9Renderer::AdjustWindowForChange()
{
#if defined(WIN32)
    if (IsEditorMode())
    {
        return S_OK;
    }

#if defined(OPENGL)
    const DXGI_SWAP_CHAIN_DESC& swapChainDesc(m_devInfo.SwapChainDesc());

    DXGI_MODE_DESC modeDesc;
    modeDesc.Width            = m_backbufferWidth;
    modeDesc.Height           = m_backbufferHeight;
    modeDesc.RefreshRate      = swapChainDesc.BufferDesc.RefreshRate;
    modeDesc.Format           = swapChainDesc.BufferDesc.Format;
    modeDesc.ScanlineOrdering = swapChainDesc.BufferDesc.ScanlineOrdering;
    modeDesc.Scaling          = swapChainDesc.BufferDesc.Scaling;

    HRESULT result = m_pSwapChain->ResizeTarget(&modeDesc);
    if (FAILED(result))
    {
        return result;
    }
#elif defined(WIN32)
    bool bFullscreenWindow = false;

    bFullscreenWindow = CV_r_FullscreenWindow && CV_r_FullscreenWindow->GetIVal() != 0;

    if (!m_bFullScreen && !bFullscreenWindow)
    {
        // Set windowed-mode style
        SetWindowLongW(m_hWnd, GWL_STYLE, m_dwWindowStyle);
    }
    else
    {
        // Set fullscreen-mode style
        SetWindowLongW(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    }

    if (m_bFullScreen)
    {
        int x = m_prefMonX;
        int y = m_prefMonY;
#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
        UserOverrideDXGIOutputFS(m_devInfo, CV_r_overrideDXGIOutputFS, m_prefMonX, m_prefMonY, x, y);
#endif
        const int wdt = m_backbufferWidth;
        const int hgt = m_backbufferHeight;
        SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, wdt, hgt, SWP_SHOWWINDOW);
    }
    else if (bFullscreenWindow)
    {
        const int x = m_prefMonX + (m_prefMonWidth - m_backbufferWidth) / 2;
        const int y = m_prefMonY + (m_prefMonHeight - m_backbufferHeight) / 2;
        const int wdt = m_backbufferWidth;
        const int hgt = m_backbufferHeight;
        SetWindowPos(m_hWnd, HWND_NOTOPMOST, x, y, wdt, hgt, SWP_SHOWWINDOW);
    }
    else
    {
        RECT wndrect;
        SetRect(&wndrect, 0, 0, m_backbufferWidth, m_backbufferHeight);
        AdjustWindowRectEx(&wndrect, m_dwWindowStyle, FALSE, WS_EX_APPWINDOW);

        const int wdt = wndrect.right  - wndrect.left;
        const int hgt = wndrect.bottom - wndrect.top;

        const int x = m_prefMonX + (m_prefMonWidth - wdt) / 2;
        const int y = m_prefMonY + (m_prefMonHeight - hgt) / 2;

        SetWindowPos(m_hWnd, HWND_NOTOPMOST, x, y, wdt, hgt, SWP_SHOWWINDOW);
    }
 #endif

    //set viewport to ensure we have a valid one, even when doing chainloading
    // and playing a video before going ingame
    m_MainViewport.nX = 0;
    m_MainViewport.nY = 0;
    m_MainViewport.nWidth = m_width;
    m_MainViewport.nHeight = m_height;
    m_MainRTViewport.nX = 0;
    m_MainRTViewport.nY = 0;
    m_MainRTViewport.nWidth = m_width;
    m_MainRTViewport.nHeight = m_height;

    m_FullResRect.right = m_width;
    m_FullResRect.bottom = m_height;

    m_pRT->RC_SetViewport(0, 0, m_width, m_height);
 #endif

    return S_OK;
}

#if defined(SUPPORT_DEVICE_INFO)
bool compareDXGIMODEDESC(const DXGI_MODE_DESC& lhs, const DXGI_MODE_DESC& rhs)
{
    if (lhs.Width != rhs.Width)
    {
        return lhs.Width < rhs.Width;
    }
    return lhs.Height < rhs.Height;
}
#endif

int CD3D9Renderer::EnumDisplayFormats([[maybe_unused]] SDispFormat* formats)
{
#if defined(WIN32) || defined(WIN64) || defined(OPENGL)

#if defined(SUPPORT_DEVICE_INFO)

    unsigned int numModes = 0;
    if (m_devInfo.Output())
    {
        if (SUCCEEDED(m_devInfo.Output()->GetDisplayModeList(m_devInfo.SwapChainDesc().BufferDesc.Format, 0, &numModes, 0)) && numModes)
        {
            std::vector<DXGI_MODE_DESC> dispModes(numModes);
            if (SUCCEEDED(m_devInfo.Output()->GetDisplayModeList(m_devInfo.SwapChainDesc().BufferDesc.Format, 0, &numModes, &dispModes[0])) && numModes)
            {
                std::sort(dispModes.begin(), dispModes.end(), compareDXGIMODEDESC);

                unsigned int numUniqueModes = 0;
                unsigned int prevWidth = 0;
                unsigned int prevHeight = 0;
                for (unsigned int i = 0; i < numModes; ++i)
                {
                    if (prevWidth != dispModes[i].Width || prevHeight != dispModes[i].Height)
                    {
                        if (formats)
                        {
                            formats[numUniqueModes].m_Width = dispModes[i].Width;
                            formats[numUniqueModes].m_Height = dispModes[i].Height;
                            formats[numUniqueModes].m_BPP = CTexture::BytesPerBlock(CTexture::TexFormatFromDeviceFormat(dispModes[i].Format)) * 8;
                        }

                        prevWidth = dispModes[i].Width;
                        prevHeight = dispModes[i].Height;
                        ++numUniqueModes;
                    }
                }

                numModes = numUniqueModes;
            }
        }
    }

    return numModes;

#endif

#else
    return 0;
#endif
}

bool CD3D9Renderer::ChangeDisplay([[maybe_unused]] unsigned int width, [[maybe_unused]] unsigned int height, [[maybe_unused]] unsigned int cbpp)
{
    return false;
}


void CD3D9Renderer::UnSetRes()
{
    m_Features |= RFT_SUPPORTZBIAS;

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
    m_d3dDebug.Release();
#endif
}

void CD3D9Renderer::DestroyWindow(void)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(OPENGL) // Scrubber safe exclusion
#else
    SAFE_RELEASE(m_DeviceContext);
#endif

    SAFE_RELEASE(m_Device);

#if defined(CRY_USE_METAL)
    DXGLDestroyMetalWindow(m_hWnd);
#elif defined(WIN32)
    if (gEnv && gEnv->pSystem)
    {
        if (m_registeredWindoWHandler)
        {
            gEnv->pSystem->UnregisterWindowMessageHandler(this);
        }
        m_registeredWindoWHandler = false;
    }
    if (m_hWnd)
    {
        ::DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
    if (m_hWnd2)
    {
        ::DestroyWindow(m_hWnd2);
        m_hWnd2 = NULL;
    }
    if (m_hIconBig)
    {
        ::DestroyIcon(m_hIconBig);
        m_hIconBig = NULL;
    }
    if (m_hIconSmall)
    {
        ::DestroyIcon(m_hIconSmall);
        m_hIconSmall = NULL;
    }
#elif defined(OPENGL)
    DXGLDestroyWindow(m_hWnd);
#endif
}

struct CD3D9Renderer::gammaramp_t
{
    uint16 red[256];
    uint16 green[256];
    uint16 blue[256];
};

static CD3D9Renderer::gammaramp_t orgGamma;

static BOOL g_doGamma = false;


void CD3D9Renderer::RestoreGamma(void)
{
    if (!(GetFeatures() & RFT_HWGAMMA))
    {
        return;
    }

    if (CV_r_nohwgamma && m_nLastNoHWGamma)
    {
        return;
    }

    m_nLastNoHWGamma = CV_r_nohwgamma;
    m_fLastGamma = 1.0f;
    m_fLastBrightness = 0.5f;
    m_fLastContrast = 0.5f;

    //iLog->Log("...RestoreGamma");

#if defined(WIN32)
    if (!g_doGamma)
    {
        return;
    }

    g_doGamma = false;

    m_hWndDesktop = GetDesktopWindow();

    if (HDC dc = GetDC(m_hWndDesktop))
    {
        SetDeviceGammaRamp(dc, &orgGamma);
        ReleaseDC(m_hWndDesktop, dc);
    }
#endif
}

void CD3D9Renderer::GetDeviceGamma()
{
#if defined(WIN32)
    if (g_doGamma)
    {
        return;
    }

    m_hWndDesktop = GetDesktopWindow();

    if (HDC dc = GetDC(m_hWndDesktop))
    {
        g_doGamma = true;

        if (!GetDeviceGammaRamp(dc, &orgGamma))
        {
            for (uint16 i = 0; i < 256; i++)
            {
                orgGamma.red  [i] = i * 0x101;
                orgGamma.green[i] = i * 0x101;
                orgGamma.blue [i] = i * 0x101;
            }
        }

        ReleaseDC(m_hWndDesktop, dc);
    }
#endif
}

void CD3D9Renderer::SetDeviceGamma([[maybe_unused]] gammaramp_t* gamma)
{
    if (!(GetFeatures() & RFT_HWGAMMA))
    {
        return;
    }

    if IsCVarConstAccess(constexpr) (bool(CV_r_nohwgamma))
    {
        return;
    }

#if defined(WIN32)
    if (!g_doGamma)
    {
        return;
    }

    m_hWndDesktop = GetDesktopWindow();  // TODO: DesktopWindow - does not represent actual output window thus gamma affects all desktop monitors !!!

    if (HDC dc = GetDC(m_hWndDesktop))
    {
        g_doGamma = true;
        // INFO!!! - very strange: in the same time
        // GetDeviceGammaRamp -> TRUE
        // SetDeviceGammaRamp -> FALSE but WORKS!!!
        // at least for desktop window DC... be careful
        SetDeviceGammaRamp(dc, gamma);
        ReleaseDC(m_hWndDesktop, dc);
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::SetGamma(float fGamma, float fBrightness, float fContrast, bool bForce)
{
    // Early out if HW gamma is disabled (same early out as SetDeviceGamma)
    if IsCVarConstAccess(constexpr) (bool(CV_r_nohwgamma))
    {
        // Restore default if HW gamma was previously enabled
        if (m_nLastNoHWGamma == 0)
        {
            RestoreGamma();
        }
        return;
    }

    if (m_pStereoRenderer)  // Function can be called on shutdown
    {
        fGamma += GetS3DRend().GetGammaAdjustment();
    }

    fGamma = CLAMP(fGamma, 0.4f, 1.6f);

    if (!bForce && m_fLastGamma == fGamma && m_fLastBrightness == fBrightness && m_fLastContrast == fContrast && m_nLastNoHWGamma == CV_r_nohwgamma)
    {
        return;
    }

    GetDeviceGamma();

    gammaramp_t gamma;

    float fInvGamma = 1.f / fGamma;

    float fAdd = (fBrightness - 0.5f) * 0.5f - fContrast * 0.5f + 0.25f;
    float fMul = fContrast + 0.5f;

    for (int i = 0; i < 256; i++)
    {
        float pfInput[3];

        pfInput[0] = (float)(orgGamma.red  [i] >> 8) / 255.f;
        pfInput[1] = (float)(orgGamma.green[i] >> 8) / 255.f;
        pfInput[2] = (float)(orgGamma.blue [i] >> 8) / 255.f;

        pfInput[0] = pow_tpl(pfInput[0], fInvGamma) * fMul + fAdd;
        pfInput[1] = pow_tpl(pfInput[1], fInvGamma) * fMul + fAdd;
        pfInput[2] = pow_tpl(pfInput[2], fInvGamma) * fMul + fAdd;

        gamma.red  [i] = CLAMP(int_round(pfInput[0] * 65535.f), 0, 65535);
        gamma.green[i] = CLAMP(int_round(pfInput[1] * 65535.f), 0, 65535);
        gamma.blue [i] = CLAMP(int_round(pfInput[2] * 65535.f), 0, 65535);
    }

    SetDeviceGamma(&gamma);

    m_nLastNoHWGamma = CV_r_nohwgamma;
    m_fLastGamma = fGamma;
    m_fLastBrightness = fBrightness;
    m_fLastContrast = fContrast;
}

bool CD3D9Renderer::SetGammaDelta(const float fGamma)
{
    m_fDeltaGamma = fGamma;
    SetGamma(CV_r_gamma + fGamma, CV_r_brightness, CV_r_contrast, false);
    return true;
}

SDepthTexture::~SDepthTexture()
{
}

void SDepthTexture::Release(bool bReleaseTex)
{
    SAFE_RELEASE(pSurf);
    if (bReleaseTex && pTarget)
    {
        gcpRendD3D->m_DevMan.ReleaseD3D11Texture2D(static_cast<ID3D11Texture2D*>(pTarget));
        pTex = nullptr;
    }
}


void CD3D9Renderer::ShutDownFast()
{
    // Flush RT command buffer
    ForceFlushRTCommands();
    CHWShader::mfFlushPendedShadersWait(-1);
    FX_PipelineShutdown(true);
    //CBaseResource::ShutDown();
    memset(&CTexture::s_TexStages[0], 0, sizeof(CTexture::s_TexStages));
    for (uint32 i = 0; i < CTexture::s_TexStates.size(); i++)
    {
        memset(&CTexture::s_TexStates[i], 0, sizeof(STexState));
    }
    SAFE_DELETE(m_pRT);

#if defined(OPENGL)
#if !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    if (CV_r_multithreaded)
    {
        DXGLReleaseContext(m_devInfo.Device());
    }
#endif //!DXGL_FULL_EMULATION
    m_devInfo.Release();
#endif //defined(OPENGL)
}

void CD3D9Renderer::RT_ShutDown(uint32 nFlags)
{
    m_volumetricFog.DestroyResources(true);

    SAFE_DELETE(m_pColorGradingControllerD3D);
    SAFE_DELETE(m_pPostProcessMgr);
    SAFE_DELETE(m_pWaterSimMgr);
    SAFE_DELETE(m_pStereoRenderer);
    SAFE_DELETE(m_pPipelineProfiler);

    m_PerInstanceConstantBufferPool.Shutdown();

    for (size_t bt = 0; bt < eBoneType_Count; ++bt)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            while (m_CharCBActiveList[bt][i].next != &m_CharCBActiveList[bt][i])
            {
                delete m_CharCBActiveList[bt][i].next->item<&SCharInstCB::list>();
            }
        }
        while (m_CharCBFreeList[bt].next != &m_CharCBFreeList[bt])
        {
            delete m_CharCBFreeList[bt].next->item<&SCharInstCB::list>();
        }
    }

    CHWShader::mfFlushPendedShadersWait(-1);
    if (nFlags == FRR_ALL)
    {
        memset(&CTexture::s_TexStages[0], 0, sizeof(CTexture::s_TexStages));
        CTexture::s_TexStates.clear();
        FreeResources(FRR_ALL);
    }

    FX_PipelineShutdown();

#if defined(SUPPORT_DEVICE_INFO)
    //m_devInfo.Release();
#  if defined(OPENGL) && !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    m_pRT->m_kDXGLDeviceContextHandle.Set(NULL, !CV_r_multithreaded);
    m_pRT->m_kDXGLContextHandle.Set(NULL);
#  endif //if defined(OPENGL) && !DXGL_FULL_EMULATION
#endif


    SAFE_RELEASE(m_pZBufferReadOnlyDSV);
    SAFE_RELEASE(m_pZBufferDepthReadOnlySRV);
    SAFE_RELEASE(m_pZBufferStencilReadOnlySRV);

    SAFE_DELETE(m_GraphicsPipeline);

#if defined(ENABLE_RENDER_AUX_GEOM)
    SAFE_DELETE(m_pRenderAuxGeomD3D);
#endif
    m_DepthBufferOrig.pSurf = NULL;
    m_DepthBufferOrig.pTex = NULL;
    m_DepthBufferOrigMSAA.pSurf = NULL;
    m_DepthBufferOrigMSAA.pTex = NULL;
    m_DepthBufferNative.pSurf = NULL;
    m_DepthBufferNative.pTex = NULL;
}

void CD3D9Renderer::ShutDown(bool bReInit)
{
    m_bInShutdown = true;

    // Force Flush RT command buffer
    ForceFlushRTCommands();
    PreShutDown();
    CWaterRipples::ReleasePhysCallbacks();
    if (m_pRT)
    {
        m_pRT->RC_ShutDown(bReInit ? (FRR_SHADERS | FRR_TEXTURES | FRR_REINITHW) : FRR_ALL);
    }

    //CBaseResource::ShutDown();
    ForceFlushRTCommands();

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
    //////////////////////////////////////////////////////////////////////////
    // Clear globals.
    //////////////////////////////////////////////////////////////////////////

    STLALLOCATOR_CLEANUP

        SAFE_DELETE(m_pRT);

#if defined(OPENGL)
#if !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    if (CV_r_multithreaded)
    {
        DXGLReleaseContext(&GetDevice());
    }
#endif //!DXGL_FULL_EMULATION
    m_devInfo.Release();
#endif //defined(OPENGL)

    if (!bReInit)
    {
        iLog = NULL;
        //iConsole = NULL;
        iTimer = NULL;
        iSystem = NULL;
    }

    EnableGPUTimers2(false);
    AllowGPUTimers2(false);

#if defined(OPENGL) && !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    DXGLFinalize();
#endif //defined(OPENGL) && !DXGL_FULL_EMULATION

    PostShutDown();
}

#ifdef WIN32
LRESULT CALLBACK LowLevelKeyboardProc (INT nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*) lParam;
    switch (nCode)
    {
    case HC_ACTION:
    {
        if (pkbhs->vkCode == VK_TAB && pkbhs->flags & LLKHF_ALTDOWN)
        {
            return 1;                // Disable ALT+ESC
        }
    }
    default:
        break;
    }
    return CallNextHookEx (0, nCode, wParam, lParam);
}
#endif

HWND CD3D9Renderer::CreateWindowCallback()
{
    gcpRendD3D->SetWindow(gcpRendD3D->GetBackbufferWidth(), gcpRendD3D->GetBackbufferHeight(), gcpRendD3D->m_bFullScreen, gcpRendD3D->m_hWnd);

    return gcpRendD3D->m_hWnd;
}

bool CD3D9Renderer::SetWindow([[maybe_unused]] int width, [[maybe_unused]] int height, [[maybe_unused]] bool fullscreen, [[maybe_unused]] WIN_HWND hWnd)
{
    LOADING_TIME_PROFILE_SECTION;

#if D3DSYSTEM_CPP_TRAIT_SETWINDOW_REGISTERWINDOWMESSAGEHANDLER
    iSystem->RegisterWindowMessageHandler(this);
    m_registeredWindoWHandler = true;
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_22
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif

#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(CRY_USE_METAL)
    if (gcpRendD3D->m_hWnd == 0)
    {
        DXGLCreateMetalWindow(m_WinTitle, width, height, fullscreen, &m_hWnd);
    }
#elif defined(WIN32)
    DWORD style, exstyle;
    int x, y, wdt, hgt;

    if (width < 640)
    {
        width = 640;
    }
    if (height < 480)
    {
        height = 480;
    }

    m_dwWindowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

    // Do not allow the user to resize the window
    m_dwWindowStyle &= ~WS_MAXIMIZEBOX;
    m_dwWindowStyle &= ~WS_THICKFRAME;

    bool bFullscreenWindow = false;
#if defined(WIN32) || defined(WIN64)
    bFullscreenWindow = CV_r_FullscreenWindow && CV_r_FullscreenWindow->GetIVal() != 0;
#endif

    if (fullscreen || bFullscreenWindow)
    {
        exstyle = bFullscreenWindow ? WS_EX_APPWINDOW : WS_EX_TOPMOST;
        style = WS_POPUP | WS_VISIBLE;
        x = m_prefMonX + (m_prefMonWidth - width) / 2;
        y = m_prefMonY + (m_prefMonHeight - height) / 2;
        wdt = width;
        hgt = height;
    }
    else
    {
        exstyle = WS_EX_APPWINDOW;
        style = m_dwWindowStyle;

        RECT wndrect;
        SetRect(&wndrect, 0, 0, width, height);
        AdjustWindowRectEx(&wndrect, style, FALSE, exstyle);

        wdt = wndrect.right  - wndrect.left;
        hgt = wndrect.bottom - wndrect.top;

        x = m_prefMonX + (m_prefMonWidth - wdt) / 2;
        y = m_prefMonY + (m_prefMonHeight - hgt) / 2;
    }

    if (IsEditorMode())
    {
#if defined(UNICODE) || defined(_UNICODE)
#error Review this, probably should be wide if Editor also has UNICODE support (or maybe moved into Editor)
#endif
        m_dwWindowStyle = WS_OVERLAPPED;
        style = m_dwWindowStyle;
        exstyle = 0;
        x = y = 0;
        wdt = 100;
        hgt = 100;

        WNDCLASSA wc;
        memset(&wc, 0, sizeof(WNDCLASSA));
        wc.style         = CS_OWNDC;
        wc.lpfnWndProc   = DefWindowProcA;
        wc.hInstance     = m_hInst;
        wc.lpszClassName = "D3DDeviceWindowClassForSandbox";
        if (!RegisterClassA(&wc))
        {
            CryFatalError("Cannot Register Window Class %s", wc.lpszClassName);
            return false;
        }
        m_hWnd = CreateWindowExA(exstyle, wc.lpszClassName, m_WinTitle, style, x, y, wdt, hgt, NULL, NULL, m_hInst, NULL);
        ShowWindow(m_hWnd, SW_HIDE);
    }
    else
    {
        if (!hWnd)
        {
            LPCWSTR pClassName = L"CryENGINE";

            // Load default icon if we have nothing yet
            if (m_hIconBig == NULL)
            {
                SetWindowIcon("textures/default_icon.dds");
            }

            // Moved from Game DLL
            WNDCLASSEXW wc;
            memset(&wc, 0, sizeof(WNDCLASSEXW));
            wc.cbSize        = sizeof(WNDCLASSEXW);
            wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            wc.lpfnWndProc   = (WNDPROC)GetISystem()->GetRootWindowMessageHandler();
            wc.hInstance     = m_hInst;
            wc.hIcon         = m_hIconBig;
            wc.hIconSm       = m_hIconSmall;
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            wc.lpszClassName = pClassName;
            if (!RegisterClassExW(&wc))
            {
                CryFatalError("Cannot Register Launcher Window Class");
                return false;
            }

            wstring wideTitle = Unicode::Convert<wstring>(m_WinTitle);

            m_hWnd = CreateWindowExW(exstyle, pClassName, wideTitle.c_str(), style, x, y, wdt, hgt, NULL, NULL, m_hInst, NULL);
            if (m_hWnd && !IsWindowUnicode(m_hWnd))
            {
                CryFatalError("Expected an UNICODE window for launcher");
                return false;
            }

            // Create second window for stereo (multi-head device)
            if (GetS3DRend().GetStereoDevice() == STEREO_DEVICE_DUALHEAD && fullscreen)
            {
                m_hWnd2 = CreateWindowExW(exstyle, pClassName, wideTitle.c_str(), style, x, y, wdt, hgt, m_hWnd, NULL, m_hInst, NULL);
            }
            else
            {
                m_hWnd2 = 0;
            }

            EnableCloseButton(m_hWnd, false);

            if (fullscreen && (!gEnv->pSystem->IsDevMode() && CV_r_enableAltTab == 0))
            {
                SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
            }
        }
        else
        {
            m_hWnd = (HWND)hWnd;
        }

        ShowWindow(m_hWnd, SW_SHOWNORMAL);
        SetFocus(m_hWnd);
        SetForegroundWindow(m_hWnd);
    }

    if (!m_hWnd)
    {
        iConsole->Exit("Couldn't create window\n");
    }
#elif defined(OPENGL)
    return DXGLCreateWindow(m_WinTitle, width, height, fullscreen, &m_hWnd);
#else
    return false;
#endif //WIN32
    return true;
}

bool CD3D9Renderer::SetWindowIcon([[maybe_unused]] const char* path)
{
#ifdef WIN32
    if (IsEditorMode())
    {
        return false;
    }

    if (azstricmp(path, m_iconPath.c_str()) == 0)
    {
        return true;
    }

    HICON hIconBig = CreateResourceFromTexture(this, path, eResourceType_IconBig);
    if (hIconBig)
    {
        if (m_hWnd)
        {
            SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
        }
        if (m_hWnd2)
        {
            SendMessage(m_hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
        }
        if (m_hIconBig)
        {
            ::DestroyIcon(m_hIconBig);
        }
        m_hIconBig = hIconBig;
        m_iconPath = path;
    }

    // Note: Also set the small icon manually.
    // Even though the big icon will also affect the small icon, the re-scaling done by GDI has aliasing problems.
    // Just grabbing a smaller MIP from the texture (if possible) will solve this.
    HICON hIconSmall = CreateResourceFromTexture(this, path, eResourceType_IconSmall);
    if (hIconSmall)
    {
        if (m_hWnd)
        {
            SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
        }
        if (m_hWnd)
        {
            SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
        }
        if (m_hIconSmall)
        {
            ::DestroyIcon(m_hIconSmall);
        }
        m_hIconSmall = hIconSmall;
    }

    return hIconBig != NULL;
#else
    return false;
#endif
}

#define QUALITY_VAR(name)                                                   \
    static void OnQShaderChange_Shader##name(ICVar * pVar)                  \
    {                                                                       \
        int iQuality = eSQ_Low;                                             \
        if (gRenDev->GetFeatures() & (RFT_HW_SM2X | RFT_HW_SM30)) {         \
            iQuality = CLAMP(pVar->GetIVal(), 0, eSQ_Max); }                \
        gRenDev->EF_SetShaderQuality(eST_##name, (EShaderQuality)iQuality); \
    }

QUALITY_VAR(General)
QUALITY_VAR(Metal)
QUALITY_VAR(Glass)
QUALITY_VAR(Ice)
QUALITY_VAR(Shadow)
QUALITY_VAR(Water)
QUALITY_VAR(FX)
QUALITY_VAR(PostProcess)
QUALITY_VAR(HDR)
QUALITY_VAR(Sky)

#undef QUALITY_VAR

static void OnQShaderChange_Renderer(ICVar* pVar)
{
    int iQuality = eRQ_Low;

    if (gRenDev->GetFeatures() & (RFT_HW_SM2X | RFT_HW_SM30))
    {
        iQuality = CLAMP(pVar->GetIVal(), 0, eSQ_Max);
    }
    else
    {
        pVar->ForceSet("0");
    }

    gRenDev->m_RP.m_eQuality = (ERenderQuality)iQuality;
}


static void Command_Quality(IConsoleCmdArgs* Cmd)
{
    bool bLog = false;
    bool bSet = false;

    int iQuality = -1;

    if (Cmd->GetArgCount() == 2)
    {
        iQuality = CLAMP(atoi(Cmd->GetArg(1)), eSQ_Low, eSQ_VeryHigh);
        bSet = true;
    }
    else
    {
        bLog = true;
    }

    if (bLog)
    {
        iLog->LogWithType(IMiniLog::eInputResponse, " ");
    }
    if (bLog)
    {
        iLog->LogWithType(IMiniLog::eInputResponse, "Current quality settings (0=low/1=med/2=high/3=very high):");
    }

#define QUALITY_VAR(name) if (bLog) {iLog->LogWithType(IMiniLog::eInputResponse, "  $3q_"#name " = $6%d", gEnv->pConsole->GetCVar("q_"#name)->GetIVal()); } \
    if (bSet) { gEnv->pConsole->GetCVar("q_"#name)->Set(iQuality); }

    QUALITY_VAR(ShaderGeneral)
    QUALITY_VAR(ShaderMetal)
    QUALITY_VAR(ShaderGlass)
    QUALITY_VAR(ShaderVegetation)
    QUALITY_VAR(ShaderIce)
    QUALITY_VAR(ShaderTerrain)
    QUALITY_VAR(ShaderShadow)
    QUALITY_VAR(ShaderWater)
    QUALITY_VAR(ShaderFX)
    QUALITY_VAR(ShaderPostProcess)
    QUALITY_VAR(ShaderHDR)
    QUALITY_VAR(ShaderSky)
    QUALITY_VAR(Renderer)

#undef QUALITY_VAR

    if (bSet)
    {
        iLog->LogWithType(IMiniLog::eInputResponse, "Set quality to %d", iQuality);
    }
}

const char* sGetSQuality(const char* szName)
{
    ICVar* pVar = iConsole->GetCVar(szName);
    assert(pVar);
    if (!pVar)
    {
        return "Unknown";
    }
    int iQ = pVar->GetIVal();
    switch (iQ)
    {
    case eSQ_Low:
        return "Low";
    case eSQ_Medium:
        return "Medium";
    case eSQ_High:
        return "High";
    case eSQ_VeryHigh:
        return "VeryHigh";
    default:
        return "Unknown";
    }
}

static void Command_ColorGradingChartImage(IConsoleCmdArgs* pCmd)
{
    CColorGradingControllerD3D* pCtrl = gcpRendD3D->m_pColorGradingControllerD3D;
    if (pCmd && pCtrl)
    {
        const int numArgs = pCmd->GetArgCount();
        if (numArgs == 1)
        {
            const CTexture* pChart = pCtrl->GetStaticColorChart();
            if (pChart)
            {
                iLog->Log("current static chart is \"%s\"", pChart->GetName());
            }
            else
            {
                iLog->Log("no static chart loaded");
            }
        }
        else if (numArgs == 2)
        {
            const char* pArg = pCmd->GetArg(1);
            if (pArg && pArg[0])
            {
                if (pArg[0] == '0' && !pArg[1])
                {
                    pCtrl->LoadStaticColorChart(0);
                    iLog->Log("static chart reset");
                }
                else
                {
                    if (pCtrl->LoadStaticColorChart(pArg))
                    {
                        iLog->Log("\"%s\" loaded successfully", pArg);
                    }
                    else
                    {
                        iLog->Log("failed to load \"%s\"", pArg);
                    }
                }
            }
        }
    }
}

WIN_HWND CD3D9Renderer::Init([[maybe_unused]] int x, [[maybe_unused]] int y, int width, int height, unsigned int cbpp, int zbpp, int sbits, bool fullscreen, bool isEditor, WIN_HINSTANCE hinst, WIN_HWND Glhwnd, [[maybe_unused]] bool bReInit, [[maybe_unused]] const SCustomRenderInitArgs* pCustomArgs, bool bShaderCacheGen)
{
    LOADING_TIME_PROFILE_SECTION;

    if (!iSystem || !iLog)
    {
        AZ_Error("CD3D9Renderer::Init", iSystem, "Renderer initialization failed because iSystem was null.");
        AZ_Error("CD3D9Renderer::Init", iLog, "Renderer initialization failed because iLog was null.");
        return 0;
    }

    iLog->Log("Initializing Direct3D and creating game window:");
    INDENT_LOG_DURING_SCOPE();

    m_CVWidth = iConsole->GetCVar("r_Width");
    m_CVHeight = iConsole->GetCVar("r_Height");
    m_CVFullScreen = iConsole->GetCVar("r_Fullscreen");
    m_CVDisplayInfo = iConsole->GetCVar("r_DisplayInfo");
    m_CVColorBits = iConsole->GetCVar("r_ColorBits");

    bool bNativeResolution;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(IOS)
    bNativeResolution = true;
#elif defined(ANDROID)
    bNativeResolution = true;
#elif defined(WIN32) || defined(WIN64)
    CV_r_FullscreenWindow = iConsole->GetCVar("r_FullscreenWindow");
    m_fullscreenWindow = CV_r_FullscreenWindow && CV_r_FullscreenWindow->GetIVal();
    CV_r_FullscreenNativeRes = iConsole->GetCVar("r_FullscreenNativeRes");
    bNativeResolution = CV_r_FullscreenNativeRes && CV_r_FullscreenNativeRes->GetIVal() != 0 && (fullscreen || m_fullscreenWindow);

    {
        RECT rcDesk;
        GetWindowRect(GetDesktopWindow(), &rcDesk);

        m_prefMonX = rcDesk.left;
        m_prefMonY = rcDesk.top;
        m_prefMonWidth = rcDesk.right - rcDesk.left;
        m_prefMonHeight = rcDesk.bottom - rcDesk.top;
    }
    {
        RECT rc;
        HDC hdc = GetDC(NULL);
        GetClipBox(hdc, &rc);
        ReleaseDC(NULL, hdc);
        m_deskwidth = rc.right  - rc.left;
        m_deskheight = rc.bottom - rc.top;
    }
#else
    bNativeResolution = false;
#endif

#if defined(OPENGL) && !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    DXGLInitialize(CV_r_multithreaded ? 4 : 0);
#endif //defined(OPENGL) && !DXGL_FULL_EMULATION

#ifdef D3DX_SDK_VERSION
    iLog->Log("D3DX_SDK_VERSION = %d", D3DX_SDK_VERSION);
#else
    iLog->Log("D3DX_SDK_VERSION = <UNDEFINED>");
#endif

    iLog->Log ("Direct3D driver is creating...");
    iLog->Log ("Crytek Direct3D driver version %4.2f (%s <%s>)", VERSION_D3D, __DATE__, __TIME__);

    auto projectName = AZ::Utils::GetProjectName();
    cry_strcpy(m_WinTitle, projectName.c_str());

    iLog->Log ("Creating window called '%s' (%dx%d)", m_WinTitle, width, height);

    m_hInst = (HINSTANCE)(TRUNCATE_PTR)hinst;

    m_bEditor = isEditor;
    if (isEditor)
    {
        fullscreen = false;
    }

    m_bShaderCacheGen = bShaderCacheGen;

    m_cbpp   = cbpp;
    m_zbpp   = zbpp;
    m_sbpp   = sbits;
    m_bFullScreen = fullscreen;

    CalculateResolutions(width, height, bNativeResolution, &m_width, &m_height, &m_nativeWidth, &m_nativeHeight, &m_backbufferWidth, &m_backbufferHeight);

    // only create device if we are not in shader cache generation mode
    if (!m_bShaderCacheGen)
    {
        // call init stereo before device is created!
        m_pStereoRenderer->InitDeviceBeforeD3D();

        while (true)
        {
            m_hWnd = (HWND)Glhwnd;

            // Creates Device here.
            bool bRes = m_pRT->RC_CreateDevice();
            if (!bRes)
            {
                ShutDown(true);
                return 0;
            }


            break;
        }

# if defined(SUPPORT_DEVICE_INFO)
        iLog->Log(" ****** D3D11 CryRender Stats ******");
        iLog->Log(" Driver description: %S", m_devInfo.AdapterDesc().Description);

        switch (m_devInfo.FeatureLevel())
        {
        case D3D_FEATURE_LEVEL_9_1:
            iLog->Log(" Feature level: DirectX 9.1");
            break;
        case D3D_FEATURE_LEVEL_9_2:
            iLog->Log(" Feature level: DirectX 9.2");
            break;
        case D3D_FEATURE_LEVEL_9_3:
            iLog->Log(" Feature level: DirectX 9.3");
            break;
        case D3D_FEATURE_LEVEL_10_0:
            iLog->Log(" Feature level: DirectX 10.0");
            break;
        case D3D_FEATURE_LEVEL_10_1:
            iLog->Log(" Feature level: DirectX 10.1");
            break;
        case D3D_FEATURE_LEVEL_11_0:
            iLog->Log(" Feature level: DirectX 11.0");
            break;
        }
        if (m_devInfo.DriverType() == D3D_DRIVER_TYPE_HARDWARE)
        {
            iLog->Log(" Rasterizer: Hardware");
        }
        else if (m_devInfo.DriverType() == D3D_DRIVER_TYPE_REFERENCE)
        {
            iLog->Log(" Rasterizer: Reference");
        }
        else if (m_devInfo.DriverType() == D3D_DRIVER_TYPE_SOFTWARE)
        {
            iLog->Log(" Rasterizer: Software");
        }

#   endif
        iLog->Log(" Current Resolution: %dx%dx%d %s", CRenderer::m_width, CRenderer::m_height, CRenderer::m_cbpp, m_bFullScreen ? "Full Screen" : "Windowed");
        iLog->Log(" HDR Rendering: %s", m_nHDRType == 1 ? "FP16" : m_nHDRType == 2 ? "MRT" : "Disabled");
        iLog->Log(" MRT HDR Rendering: %s", (m_bDeviceSupportsFP16Separate) ? "Enabled" : "Disabled");
        iLog->Log(" Occlusion queries: %s", (m_Features & RFT_OCCLUSIONTEST) ? "Supported" : "Not supported");
        iLog->Log(" Geometry instancing: %s", (m_bDeviceSupportsInstancing) ? "Supported" : "Not supported");
        iLog->Log(" Vertex textures: %s", (m_bDeviceSupportsVertexTexture) ? "Supported" : "Not supported");
        iLog->Log(" R32F rendertarget: %s", (m_bDeviceSupportsR32FRendertarget) ? "Supported" : "Not supported");
        iLog->Log(" NormalMaps compression : %s", m_hwTexFormatSupport.m_FormatBC5U.IsValid() ? "Supported" : "Not supported");
        iLog->Log(" Gamma control: %s", (m_Features & RFT_HWGAMMA) ? "Hardware" : "Software");
        iLog->Log(" Vertex Shaders version %d.%d", 4, 0);
        iLog->Log(" Pixel Shaders version %d.%d", 4, 0);

        CRenderer::ChangeGeomInstancingThreshold();     // to get log printout and to set the internal value (vendor dependent)

        m_Features |= RFT_HW_SM20 | RFT_HW_SM2X | RFT_HW_SM30;

        // Force the Z targets to be 16-bit float
        if (!m_bDeviceSupportsR32FRendertarget)
        {
            CTexture::s_eTFZ = eTF_R16F;
        }
        if (!gcpRendD3D->UseHalfFloatRenderTargets())
        {
            CTexture::s_eTFZ = eTF_R16U;
        }

        // Note: Not rolling this into the if statement above in case s_eTFZ is set to R16F on initialization
        if (CTexture::s_eTFZ != eTF_R32F)
        {
            CRenderer::CV_r_CBufferUseNativeDepth = 0; //0=disable
        }

        if (!m_bDeviceSupportsInstancing)
        {
            _SetVar("r_GeomInstancing", 0);
        }

        const char* str = NULL;
        if (m_Features & RFT_HW_SM50)
        {
            str = "SM.5.0";
        }
        else if (m_Features & RFT_HW_SM40)
        {
            str = "SM.4.0";
        }
        else
        {
            assert(0);
        }
        iLog->Log(" Shader model usage: '%s'", str);
    }
    else
    {
        // force certain features during shader cache gen mode
        m_Features |= RFT_HW_SM20 | RFT_HW_SM2X | RFT_HW_SM30;

        m_bDeviceSupportsFP16Filter = true;

#if defined(ENABLE_NULL_D3D11DEVICE)
        m_Device = new NullD3D11Device;
        D3DDeviceContext* pContext = NULL;
#if defined(DEVICE_SUPPORTS_D3D11_3)
        GetDevice().GetImmediateContext3(&pContext);
#elif defined(DEVICE_SUPPORTS_D3D11_2)
        GetDevice().GetImmediateContext2(&pContext);
#elif defined(DEVICE_SUPPORTS_D3D11_1)
        GetDevice().GetImmediateContext1(&pContext);
#else
        GetDevice().GetImmediateContext(&pContext);
#endif
        m_DeviceContext = pContext;
#endif
    }

    iLog->Log(" *****************************************");
    iLog->Log(" ");

    iLog->Log("Init Shaders");

    //  if (!(GetFeatures() & (RFT_HW_PS2X | RFT_HW_PS30)))
    //    SetShaderQuality(eST_All, eSQ_Low);

    // Quality console variables --------------------------------------

#define QUALITY_VAR(name) { ICVar* pVar = iConsole->Register("q_Shader"#name, &m_cEF.m_ShaderProfiles[(int)eST_##name].m_iShaderProfileQuality, 1,                                          \
                                    0, CVARHELP("Defines the shader quality of "#name "\nUsage: q_Shader"#name " 0=low/1=med/2=high/3=very high (default)"), OnQShaderChange_Shader##name); \
                            OnQShaderChange_Shader##name(pVar);                                                                                                                             \
                            iLog->Log(" %s shader quality: %s", #name, sGetSQuality("q_Shader"#name)); } // clamp for lowspec

    QUALITY_VAR(General);
    QUALITY_VAR(Metal);
    QUALITY_VAR(Glass);
    QUALITY_VAR(Ice);
    QUALITY_VAR(Shadow);
    QUALITY_VAR(Water);
    QUALITY_VAR(FX);
    QUALITY_VAR(PostProcess);
    QUALITY_VAR(HDR);
    QUALITY_VAR(Sky);


#undef QUALITY_VAR

    ICVar* pVar = REGISTER_INT_CB("q_Renderer", 3, 0, "Defines the quality of Renderer\nUsage: q_Renderer 0=low/1=med/2=high/3=very high (default)", OnQShaderChange_Renderer);
    OnQShaderChange_Renderer(pVar); // clamp for lowspec, report renderer current value
    iLog->Log("Render quality: %s", sGetSQuality("q_Renderer"));

    REGISTER_COMMAND("q_Quality", &Command_Quality, 0,
        "If called with a parameter it sets the quality of all q_.. variables\n"
        "otherwise it prints their current state\n"
        "Usage: q_Quality [0=low/1=med/2=high/3=very high]");

    REGISTER_COMMAND("r_ColorGradingChartImage", &Command_ColorGradingChartImage, 0,
        "If called with a parameter it loads a color chart image. This image will overwrite\n"
        " the dynamic color chart blending result and be used during post processing instead.\n"
        "If called with no parameter it displays the name of the previously loaded chart.\n"
        "To reset a previously loaded chart call r_ColorGradingChartImage 0.\n"
        "Usage: r_ColorGradingChartImage [path of color chart image/reset]");

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_7
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif

#if defined(OPENGL) && !DXGL_FULL_EMULATION && !defined(CRY_USE_METAL)
    if (!m_pRT->IsRenderThread())
    {
        DXGLUnbindDeviceContext(&GetDeviceContext(), !CV_r_multithreaded);
    }
#endif //defined(OPENGL) && !DXGL_FULL_EMULATION

    if (!bShaderCacheGen)
    {
        m_pRT->RC_Init();
    }

    if (!g_shaderGeneralHeap)
    {
        g_shaderGeneralHeap = CryGetIMemoryManager()->CreateGeneralExpandingMemoryHeap(4 * 1024 * 1024, 0, "Shader General");
    }

    m_cEF.mfInit();

    CWaterRipples::CreatePhysCallbacks();

    if (!IsEditorMode() && !IsShaderCacheGenMode())
    {
        m_pRT->RC_PrecacheDefaultShaders();
    }

    //PostInit();

#if defined(WIN32)
    // Initialize the set of connected monitors
    HandleMessage(0, WM_DEVICECHANGE, 0, 0, 0);
    m_bDisplayChanged = false;
#endif

    m_bInitialized = true;

    //  Cry_memcheck();

    if (!bShaderCacheGen)
    {
    }

    // Success, return the window handle
    return (m_hWnd);
}

//=============================================================================

int CD3D9Renderer::EnumAAFormats([[maybe_unused]] SAAFormat* formats)
{
#if defined(SUPPORT_DEVICE_INFO)

    int numFormats = 0;

    for (unsigned int i = 1; i <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++i)
    {
        unsigned int maxQuality;
        if (SUCCEEDED(m_devInfo.Device()->CheckMultisampleQualityLevels(m_devInfo.SwapChainDesc().BufferDesc.Format, i, &maxQuality)) && maxQuality > 0)
        {
            if (formats)
            {
                formats[numFormats].nSamples = i;
                formats[numFormats].nQuality = 0;
                formats[numFormats].szDescr[0] = 0;
            }

            ++numFormats;
        }
    }

    return numFormats;

#else // #if defined(SUPPORT_DEVICE_INFO)

    return 0;

#endif
}

int CD3D9Renderer::GetAAFormat(TArray<SAAFormat>& Formats)
{
    int nNums = EnumAAFormats(NULL);
    if (nNums > 0)
    {
        Formats.resize(nNums);
        EnumAAFormats(&Formats[0]);
    }

    for (unsigned int i = 0; i < Formats.Num(); i++)
    {
        if (CV_r_msaa_samples == Formats[i].nSamples && CV_r_msaa_quality == Formats[i].nQuality)
        {
            return (int) i;
        }
    }

    return -1;
}

bool CD3D9Renderer::CheckMSAAChange()
{
    bool bChanged = false;
    if (CV_r_msaa != m_MSAA || (CV_r_msaa && (m_MSAA_quality != CV_r_msaa_quality || m_MSAA_samples != CV_r_msaa_samples)))
    {
        if (CV_r_msaa && (m_hwTexFormatSupport.m_FormatR16G16B16A16.bCanMultiSampleRT || m_hwTexFormatSupport.m_FormatR16G16.bCanMultiSampleRT))
        {
            CTexture::s_eTFZ = eTF_R32F;
            TArray<SAAFormat> Formats;
            int nNum = GetAAFormat(Formats);
            if (nNum < 0)
            {
                iLog->Log(" MSAA: Requested mode not supported\n");
                _SetVar("r_MSAA", 0);
                m_MSAA = 0;
            }
            else
            {
                iLog->Log(" MSAA: Enabled %d samples (quality level %d)", Formats[nNum].nSamples, Formats[nNum].nQuality);
                if (Formats[nNum].nQuality != m_MSAA_quality || Formats[nNum].nSamples != m_MSAA_samples)
                {
                    bChanged = true;
                    _SetVar("r_MSAA_quality", Formats[nNum].nQuality);
                    _SetVar("r_MSAA_samples", Formats[nNum].nSamples);
                }
                else
                if (!m_MSAA)
                {
                    bChanged = true;
                }
            }
        }
        else
        {
            CTexture::s_eTFZ = eTF_R32F;
            bChanged = true;
            iLog->Log(" MSAA: Disabled");
        }
        m_MSAA = CV_r_msaa;
        m_MSAA_quality = CV_r_msaa_quality;
        m_MSAA_samples = CV_r_msaa_samples;
    }

    return bChanged;
}

bool CD3D9Renderer::CheckSSAAChange()
{
    const int width = m_CVWidth ? m_CVWidth->GetIVal() : m_width;
    const int height = m_CVHeight ? m_CVHeight->GetIVal() : m_height;
    int numSSAASamples = 1;
    if (width > 0 && height > 0)
    {
        const int maxSamples = min(m_MaxTextureSize / width, m_MaxTextureSize / height);
        numSSAASamples = clamp_tpl(CV_r_Supersampling, 1, maxSamples);
    }
    if (m_numSSAASamples != numSSAASamples)
    {
        m_numSSAASamples = numSSAASamples;
        return true;
    }
    return false;
}

//==========================================================================
bool CD3D9Renderer::SetRes()
{
    LOADING_TIME_PROFILE_SECTION;
    ChangeLog();

    m_pixelAspectRatio = 1.0f;

    ///////////////////////////////////////////////////////////////////
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_8
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(IOS) || defined(ANDROID)
    m_bFullScreen = true;

    if (!m_devInfo.CreateDevice(false, m_width, m_height, m_backbufferWidth, m_backbufferHeight, m_zbpp, OnD3D11CreateDevice, CreateWindowCallback))
    {
        return false;
    }
    m_devInfo.SyncInterval() = m_VSync ? 1 : 0;

    OnD3D11PostCreateDevice(m_devInfo.Device());

    AdjustWindowForChange();

    CreateContext(m_hWnd);
#elif defined(WIN32) || defined(APPLE) || defined(LINUX)

    UnSetRes();

    int width = m_width;
    int height = m_height;
    if (IsEditorMode())
    {
        // Note: Editor is a special case, m_backbufferWidth needs to be the same as m_width
        width = m_deskwidth;
        height = m_deskheight;
    }

    // DirectX9 and DirectX10 device creating
#if defined(SUPPORT_DEVICE_INFO)
    if (m_devInfo.CreateDevice(!m_bFullScreen, width, height, m_backbufferWidth, m_backbufferHeight, m_zbpp, OnD3D11CreateDevice, CreateWindowCallback))
    {
        m_devInfo.SyncInterval() = m_VSync ? 1 : 0;
    }
    else
    {
        return false;
    }

    OnD3D11PostCreateDevice(m_devInfo.Device());
#endif

    AdjustWindowForChange();

    CreateContext(m_hWnd);

#else
    #error UNKNOWN RENDER DEVICE PLATFORM
#endif

    m_DevBufMan.Init();

    m_pStereoRenderer->InitDeviceAfterD3D();

    return true;
}


bool SPixFormat::CheckSupport(D3DFormat Format, const char* szDescr, [[maybe_unused]] ETexture_Usage eTxUsage)
{
    bool bRes = true;
    CD3D9Renderer* rd = gcpRendD3D;

    UINT nOptions;
    HRESULT hr = gcpRendD3D->GetDevice().CheckFormatSupport(Format, &nOptions);
    if (SUCCEEDED(hr))
    {
        if (nOptions & (D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_TEXTURECUBE))
        {
            bool canReadSRGB = CTexture::IsDeviceFormatSRGBReadable(Format);

            //  TODO: check if need to allow other compressed formats to stay here formats here too?
            //  Adding PVRTC format here improved the picture on iOS device.

            bool bCanMips = true;

            Init();
            DeviceFormat  = Format;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_9
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            MaxWidth = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            MaxHeight = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
#endif
            Desc          = szDescr;
            BytesPerBlock = CTexture::BytesPerBlock(CTexture::TexFormatFromDeviceFormat(Format));

            bCanDS            = (nOptions & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL) != 0;
            bCanRT            = (nOptions & D3D11_FORMAT_SUPPORT_RENDER_TARGET) != 0;
            bCanMultiSampleRT = (nOptions & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET) != 0;
            bCanMips          = (nOptions & D3D11_FORMAT_SUPPORT_MIP) != 0;
            bCanMipsAutoGen   = (nOptions & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN) != 0;
            bCanGather        = (nOptions & D3D11_FORMAT_SUPPORT_SHADER_GATHER) != 0;
            bCanGatherCmp     = (nOptions & D3D11_FORMAT_SUPPORT_SHADER_GATHER_COMPARISON) != 0;
            bCanBlend         = (nOptions & D3D11_FORMAT_SUPPORT_BLENDABLE) != 0;
            bCanReadSRGB      = canReadSRGB;

            if (bCanDS || bCanRT || bCanGather || bCanBlend || bCanReadSRGB || bCanMips)
            {
                iLog->Log("  %s%s%s%s%s%s%s%s%s%s",
                    szDescr,
                    bCanMips ? ", mips" : "",
                    bCanMipsAutoGen ? " (autogen)" : "",
                    bCanReadSRGB ? ", sRGB" : "",
                    bCanBlend ? ", blend" : "",
                    bCanDS ? ", DS" : "",
                    bCanRT ? ", RT" : "",
                    bCanMultiSampleRT ? " (multi-sampled)" : "",
                    bCanGather ? ", gather" : "",
                    bCanGatherCmp ? " (comparable)" : ""
                    );
            }
            else
            {
                iLog->Log("  %s", szDescr);
            }

            Next = rd->m_hwTexFormatSupport.m_FirstPixelFormat;
            rd->m_hwTexFormatSupport.m_FirstPixelFormat = this;
        }
        else
        {
            bRes = false;
        }
    }
    else
    {
        bRes = false;
    }

    return bRes;
}

void SPixFormatSupport::CheckFormatSupport()
{
    iLog->Log("Using pixel texture formats:");

    m_FirstPixelFormat = NULL;

    m_FormatR8G8B8A8S.CheckSupport(DXGI_FORMAT_R8G8B8A8_SNORM, "R8G8B8A8S");
    m_FormatR8G8B8A8.CheckSupport(DXGI_FORMAT_R8G8B8A8_UNORM, "R8G8B8A8");

    //m_FormatR1.CheckSupport(DXGI_FORMAT_R1_UNORM, "R1");
    m_FormatA8.CheckSupport(DXGI_FORMAT_A8_UNORM, "A8");
    m_FormatR8.CheckSupport(DXGI_FORMAT_R8_UNORM, "R8");
    m_FormatR8S.CheckSupport(DXGI_FORMAT_R8_SNORM, "R8S");
    m_FormatR16.CheckSupport(DXGI_FORMAT_R16_UNORM, "R16");
    m_FormatR16U.CheckSupport(DXGI_FORMAT_R16_UINT, "R16U");
    m_FormatR16G16U.CheckSupport(DXGI_FORMAT_R16G16_UINT, "R16G16U");
    m_FormatR10G10B10A2UI.CheckSupport(DXGI_FORMAT_R10G10B10A2_UINT, "R10G10B10A2UI");
    m_FormatR16F.CheckSupport(DXGI_FORMAT_R16_FLOAT, "R16F");
    m_FormatR32F.CheckSupport(DXGI_FORMAT_R32_FLOAT, "R32F");
    m_FormatR8G8.CheckSupport(DXGI_FORMAT_R8G8_UNORM, "R8G8");
    m_FormatR8G8S.CheckSupport(DXGI_FORMAT_R8G8_SNORM, "R8G8S");
    m_FormatR16G16.CheckSupport(DXGI_FORMAT_R16G16_UNORM, "R16G16");
    m_FormatR16G16S.CheckSupport(DXGI_FORMAT_R16G16_SNORM, "R16G16S");
    m_FormatR16G16F.CheckSupport(DXGI_FORMAT_R16G16_FLOAT, "R16G16F");
    m_FormatR11G11B10F.CheckSupport(DXGI_FORMAT_R11G11B10_FLOAT, "R11G11B10F");
    m_FormatR10G10B10A2.CheckSupport(DXGI_FORMAT_R10G10B10A2_UNORM, "R10G10B10A2");
    m_FormatR16G16B16A16.CheckSupport(DXGI_FORMAT_R16G16B16A16_UNORM, "R16G16B16A16");
    m_FormatR16G16B16A16S.CheckSupport(DXGI_FORMAT_R16G16B16A16_SNORM, "R16G16B16A16S");
    m_FormatR16G16B16A16F.CheckSupport(DXGI_FORMAT_R16G16B16A16_FLOAT, "R16G16B16A16F");
    m_FormatR32G32B32A32F.CheckSupport(DXGI_FORMAT_R32G32B32A32_FLOAT, "R32G32B32A32F");

    m_FormatBC1.CheckSupport(DXGI_FORMAT_BC1_UNORM, "BC1");
    m_FormatBC2.CheckSupport(DXGI_FORMAT_BC2_UNORM, "BC2");
    m_FormatBC3.CheckSupport(DXGI_FORMAT_BC3_UNORM, "BC3");
    m_FormatBC4U.CheckSupport(DXGI_FORMAT_BC4_UNORM, "BC4");
    m_FormatBC4S.CheckSupport(DXGI_FORMAT_BC4_SNORM, "BC4S");
    m_FormatBC5U.CheckSupport(DXGI_FORMAT_BC5_UNORM, "BC5");
    m_FormatBC5S.CheckSupport(DXGI_FORMAT_BC5_SNORM, "BC5S");
    m_FormatBC6UH.CheckSupport(DXGI_FORMAT_BC6H_UF16, "BC6UH");
    m_FormatBC6SH.CheckSupport(DXGI_FORMAT_BC6H_SF16, "BC6SH");
    m_FormatBC7.CheckSupport(DXGI_FORMAT_BC7_UNORM, "BC7");
    m_FormatR9G9B9E5.CheckSupport(DXGI_FORMAT_R9G9B9E5_SHAREDEXP, "R9G9B9E5");

    // Depth formats
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_10
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    m_FormatD32FS8.CheckSupport(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, "R32FX8T");
    m_FormatD32F.CheckSupport(DXGI_FORMAT_R32_TYPELESS, "R32T");
    m_FormatD24S8.CheckSupport(DXGI_FORMAT_R24G8_TYPELESS, "R24G8T");
    m_FormatD16.CheckSupport(DXGI_FORMAT_R16_TYPELESS, "R16T");
#endif

    m_FormatB5G6R5.CheckSupport(DXGI_FORMAT_B5G6R5_UNORM, "B5G6R5");
    m_FormatB5G5R5.CheckSupport(DXGI_FORMAT_B5G5R5A1_UNORM, "B5G5R5");
    //  m_FormatB4G4R4A4.CheckSupport(DXGI_FORMAT_B4G4R4A4_UNORM, "B4G4R4A4");

    m_FormatB8G8R8A8.CheckSupport(DXGI_FORMAT_B8G8R8A8_UNORM, "B8G8R8A8");
    m_FormatB8G8R8X8.CheckSupport(DXGI_FORMAT_B8G8R8X8_UNORM, "B8G8R8X8");

#if defined(OPENGL)
    m_FormatEAC_R11.CheckSupport(DXGI_FORMAT_EAC_R11_UNORM, "EAC_R11");
    m_FormatEAC_RG11.CheckSupport(DXGI_FORMAT_EAC_RG11_UNORM, "EAC_RG11");
    m_FormatETC2.CheckSupport(DXGI_FORMAT_ETC2_UNORM, "ETC2");
    m_FormatETC2A.CheckSupport(DXGI_FORMAT_ETC2A_UNORM, "ETC2A");
#endif //defined(OPENGL)

#ifdef CRY_USE_METAL
    m_FormatPVRTC2.CheckSupport(DXGI_FORMAT_PVRTC2_UNORM, "PVRTC2");
    m_FormatPVRTC4.CheckSupport(DXGI_FORMAT_PVRTC4_UNORM, "PVRTC4");
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
    m_FormatASTC_4x4.CheckSupport(DXGI_FORMAT_ASTC_4x4_UNORM, "ASTC_4x4");
    m_FormatASTC_5x4.CheckSupport(DXGI_FORMAT_ASTC_5x4_UNORM, "ASTC_5x4");
    m_FormatASTC_5x5.CheckSupport(DXGI_FORMAT_ASTC_5x5_UNORM, "ASTC_5x5");
    m_FormatASTC_6x5.CheckSupport(DXGI_FORMAT_ASTC_6x5_UNORM, "ASTC_6x5");
    m_FormatASTC_6x6.CheckSupport(DXGI_FORMAT_ASTC_6x6_UNORM, "ASTC_6x6");
    m_FormatASTC_8x5.CheckSupport(DXGI_FORMAT_ASTC_8x5_UNORM, "ASTC_8x5");
    m_FormatASTC_8x6.CheckSupport(DXGI_FORMAT_ASTC_8x6_UNORM, "ASTC_8x6");
    m_FormatASTC_8x8.CheckSupport(DXGI_FORMAT_ASTC_8x8_UNORM, "ASTC_8x8");
    m_FormatASTC_10x5.CheckSupport(DXGI_FORMAT_ASTC_10x5_UNORM, "ASTC_10x5");
    m_FormatASTC_10x6.CheckSupport(DXGI_FORMAT_ASTC_10x6_UNORM, "ASTC_10x6");
    m_FormatASTC_10x8.CheckSupport(DXGI_FORMAT_ASTC_10x8_UNORM, "ASTC_10x8");
    m_FormatASTC_10x10.CheckSupport(DXGI_FORMAT_ASTC_10x10_UNORM, "ASTC_10x10");
    m_FormatASTC_12x10.CheckSupport(DXGI_FORMAT_ASTC_12x10_UNORM, "ASTC_12x10");
    m_FormatASTC_12x12.CheckSupport(DXGI_FORMAT_ASTC_12x12_UNORM, "ASTC_12x12");
#endif
}

void CD3D9Renderer::GetVideoMemoryUsageStats(size_t& vidMemUsedThisFrame, size_t& vidMemUsedRecently, bool bGetPoolsSizes)
{
    if (bGetPoolsSizes)
    {
        vidMemUsedThisFrame = vidMemUsedRecently = (GetTexturesStreamPoolSize() + CV_r_rendertargetpoolsize) * 1024 * 1024;
    }
    else
    {
        assert(!"CD3D9Renderer::GetVideoMemoryUsageStats() not implemented for this platform yet!");
        vidMemUsedThisFrame = vidMemUsedRecently = 0;
    }
}

//===========================================================================================

HRESULT CALLBACK CD3D9Renderer::OnD3D11CreateDevice(D3DDevice* pd3dDevice)
{
    LOADING_TIME_PROFILE_SECTION;
    CD3D9Renderer* rd = gcpRendD3D;
    rd->m_Device = pd3dDevice;

#if defined(SUPPORT_DEVICE_INFO)
    rd->m_DeviceContext = rd->m_devInfo.Context();
#endif
    rd->m_Features |= RFT_OCCLUSIONQUERY | RFT_ALLOWANISOTROPIC | RFT_HW_SM20 | RFT_HW_SM2X | RFT_HW_SM30 | RFT_HW_SM40 | RFT_HW_SM50;

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
    rd->m_d3dDebug.Init(pd3dDevice);
    rd->m_d3dDebug.Update(ESeverityCombination(CV_d3d11_debugMuteSeverity->GetIVal()), CV_d3d11_debugMuteMsgID->GetString(), CV_d3d11_debugBreakOnMsgID->GetString());
    rd->m_bUpdateD3DDebug = false;
#endif

#if defined(SUPPORT_DEVICE_INFO)
    rd->BindContextToThread(CryGetCurrentThreadId());

    LARGE_INTEGER driverVersion;
    driverVersion.LowPart = 0;
    driverVersion.HighPart = 0;
    rd->m_devInfo.Adapter()->CheckInterfaceSupport(__uuidof(ID3D10Device), &driverVersion);
    iLog->Log ("D3D Adapter: Description: %ls", rd->m_devInfo.AdapterDesc().Description);
    iLog->Log ("D3D Adapter: Driver version (UMD): %d.%02d.%02d.%04d", HIWORD(driverVersion.u.HighPart), LOWORD(driverVersion.u.HighPart), HIWORD(driverVersion.u.LowPart), LOWORD(driverVersion.u.LowPart));
    iLog->Log ("D3D Adapter: VendorId = 0x%.4X", rd->m_devInfo.AdapterDesc().VendorId);
    iLog->Log ("D3D Adapter: DeviceId = 0x%.4X", rd->m_devInfo.AdapterDesc().DeviceId);
    iLog->Log ("D3D Adapter: SubSysId = 0x%.8X", rd->m_devInfo.AdapterDesc().SubSysId);
    iLog->Log ("D3D Adapter: Revision = %i", rd->m_devInfo.AdapterDesc().Revision);

    // Vendor-specific initializations and workarounds for driver bugs.
    {
        const DXGI_ADAPTER_DESC1& adapterDesc = rd->m_devInfo.AdapterDesc();

        rd->m_adapterDescription = AZStd::string::format("%ls", adapterDesc.Description);

        if (adapterDesc.VendorId == RenderCapabilities::s_gpuVendorIdAMD)
        {
            rd->m_Features |= RFT_HW_ATI;
            iLog->Log ("D3D Detected: AMD video card");
        }
        else if (adapterDesc.VendorId == RenderCapabilities::s_gpuVendorIdNVIDIA)
        {
            rd->m_Features |= RFT_HW_NVIDIA;
            iLog->Log ("D3D Detected: NVIDIA video card");
        }
        else if (adapterDesc.VendorId == RenderCapabilities::s_gpuVendorIdQualcomm)
        {
            rd->m_Features |= RFT_HW_QUALCOMM;
            iLog->Log ("D3D Detected: Qualcomm video card");
        }
        else if (rd->m_devInfo.AdapterDesc().VendorId == RenderCapabilities::s_gpuVendorIdIntel)
        {
            rd->m_Features |= RFT_HW_INTEL;
            iLog->Log ("D3D Detected: intel video card");
        }
        else if (rd->m_devInfo.AdapterDesc().VendorId == RenderCapabilities::s_gpuVendorIdARM)
        {
            rd->m_Features |= RFT_HW_ARM_MALI;
            iLog->Log ("D3D Detected: ARM (MALI) video card");
        }

#if defined(OPENGL) && !defined(CRY_USE_METAL)
        DXGLInitializeIHVSpecifix();
#endif
    }

    rd->m_nGPUs = min(rd->m_nGPUs, (uint32)MAX_GPU_NUM);
#endif

    CryLogAlways("Active GPUs: %i", rd->m_nGPUs);

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_11
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    rd->m_NumResourceSlots = D3D11_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT;
    rd->m_NumSamplerSlots = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
    rd->m_MaxAnisotropyLevel = min(D3D11_REQ_MAXANISOTROPY, CRenderer::CV_r_texmaxanisotropy);
#endif

#if defined(WIN32) || defined(WIN64)
    HWND hWndDesktop = GetDesktopWindow();
    HDC dc = GetDC(hWndDesktop);
    uint16 gamma[3][256];
    if (GetDeviceGammaRamp(dc, gamma))
    {
        rd->m_Features |= RFT_HWGAMMA;
    }
    ReleaseDC(hWndDesktop, dc);
#endif

    // For safety, lots of drivers don't handle tiny texture sizes too tell.
#if defined(SUPPORT_DEVICE_INFO) && !defined(CRY_USE_METAL)
    rd->m_MaxTextureMemory = rd->m_devInfo.AdapterDesc().DedicatedVideoMemory;
#else
    rd->m_MaxTextureMemory = 256 * 1024 * 1024;

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_12
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#elif defined(IOS)
    rd->m_MaxTextureMemory = 1024 * 1024 * 1024;
#endif
#endif
    if (CRenderer::CV_r_TexturesStreamPoolSize <= 0)
    {
        CRenderer::CV_r_TexturesStreamPoolSize = (int)(rd->m_MaxTextureMemory / 1024.0f / 1024.0f * 0.75f);
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_13
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    rd->m_MaxTextureSize = D3D11_REQ_FILTERING_HW_ADDRESSABLE_RESOURCE_DIMENSION;
    rd->m_bDeviceSupportsInstancing = true;
#endif

    rd->m_bDeviceSupportsVertexTexture = (rd->m_Features & RFT_HW_SM30);
    if (rd->m_bDeviceSupportsVertexTexture != 0)
    {
        rd->m_Features |= RFT_HW_VERTEXTEXTURES;
    }

#if defined(CRY_USE_METAL) || (defined(OPENGL_ES))
    // METAL Supports R32 RTs but it doesn't support blending for it. Cryengine uses blending to fix up depth.
    // Disable R32 RT for metal for now.
    // Qualcomm's OpenGL ES 3.0 driver does not support R32F render targets.  Disabling for all OpenGL ES 3.0 versions for the time being.
    // When bound as a render target, we get this error:
    // W/Adreno-ES20(12623): <validate_render_targets:444>: GL_INVALID_OPERATION

    //R32 depth render targets are showing instability on Mali GPUs. For stability reasons we are forcing R16 render targets across all Android devices.
    rd->m_bDeviceSupportsR32FRendertarget = false;
#else
    rd->m_bDeviceSupportsR32FRendertarget = true;
#endif

#if defined(CRY_USE_METAL) || defined(OPENGL)
    D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS computeShadersSupport;
    HRESULT result = rd->GetDevice().CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &computeShadersSupport, sizeof(computeShadersSupport));
    if (result == S_OK && computeShadersSupport.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x == TRUE)
    {
        rd->m_Features |= RFT_COMPUTE_SHADERS;
    }
#else
    rd->m_Features |= RFT_COMPUTE_SHADERS;
#endif

    if (RenderCapabilities::SupportsStructuredBuffer(EShaderStage_Vertex))
    {
        rd->m_Features |= RFT_HW_VERTEX_STRUCTUREDBUF;
    }

#if defined(DIRECT3D10) && !defined(OPENGL_ES) && !defined(CRY_USE_METAL)
    rd->m_bDeviceSupportsGeometryShaders = (rd->m_Features & RFT_HW_SM40) != 0;
#else
    rd->m_bDeviceSupportsGeometryShaders = false;
#endif

#if defined(DIRECT3D10) && !defined(OPENGL) && !defined(CRY_USE_METAL)
    rd->m_bDeviceSupportsTessellation = (rd->m_Features & RFT_HW_SM50) != 0;
#else
    rd->m_bDeviceSupportsTessellation = false;
#endif

    rd->m_Features |= RFT_OCCLUSIONTEST;

    rd->m_bUseWaterTessHW = CV_r_WaterTessellationHW != 0 && rd->m_bDeviceSupportsTessellation;

    PREFAST_SUPPRESS_WARNING(6326); //not a constant vs constant comparison on Win32/Win64
    rd->m_bUseSilhouettePOM = CV_r_SilhouettePOM != 0;
    rd->m_bUseSpecularAntialiasing = CV_r_SpecularAntialiasing != 0;
    CV_r_DeferredShadingAmbientSClear = !(rd->m_Features & RFT_HW_NVIDIA) ? 0 : CV_r_DeferredShadingAmbientSClear;

    // Handle the texture formats we need.
    {
        // Find the needed texture formats.
        rd->m_hwTexFormatSupport.CheckFormatSupport();

        rd->m_bDeviceSupportsFP16Separate = false;
        rd->m_bDeviceSupportsFP16Filter = true;

#ifdef CRY_USE_METAL
        rd->m_FormatPVRTC2.CheckSupport(DXGI_FORMAT_PVRTC2_UNORM, "PVRTC2");
        rd->m_FormatPVRTC4.CheckSupport(DXGI_FORMAT_PVRTC4_UNORM, "PVRTC4");
#endif
#if defined(ANDROID) || defined(CRY_USE_METAL)
        rd->m_FormatASTC_4x4.CheckSupport(DXGI_FORMAT_ASTC_4x4_UNORM, "ASTC_4x4");
        rd->m_FormatASTC_5x4.CheckSupport(DXGI_FORMAT_ASTC_5x4_UNORM, "ASTC_5x4");
        rd->m_FormatASTC_5x5.CheckSupport(DXGI_FORMAT_ASTC_5x5_UNORM, "ASTC_5x5");
        rd->m_FormatASTC_6x5.CheckSupport(DXGI_FORMAT_ASTC_6x5_UNORM, "ASTC_6x5");
        rd->m_FormatASTC_6x6.CheckSupport(DXGI_FORMAT_ASTC_6x6_UNORM, "ASTC_6x6");
        rd->m_FormatASTC_8x5.CheckSupport(DXGI_FORMAT_ASTC_8x5_UNORM, "ASTC_8x5");
        rd->m_FormatASTC_8x6.CheckSupport(DXGI_FORMAT_ASTC_8x6_UNORM, "ASTC_8x6");
        rd->m_FormatASTC_8x8.CheckSupport(DXGI_FORMAT_ASTC_8x8_UNORM, "ASTC_8x8");
        rd->m_FormatASTC_10x5.CheckSupport(DXGI_FORMAT_ASTC_10x5_UNORM, "ASTC_10x5");
        rd->m_FormatASTC_10x6.CheckSupport(DXGI_FORMAT_ASTC_10x6_UNORM, "ASTC_10x6");
        rd->m_FormatASTC_10x8.CheckSupport(DXGI_FORMAT_ASTC_10x8_UNORM, "ASTC_10x8");
        rd->m_FormatASTC_10x10.CheckSupport(DXGI_FORMAT_ASTC_10x10_UNORM, "ASTC_10x10");
        rd->m_FormatASTC_12x10.CheckSupport(DXGI_FORMAT_ASTC_12x10_UNORM, "ASTC_12x10");
        rd->m_FormatASTC_12x12.CheckSupport(DXGI_FORMAT_ASTC_12x12_UNORM, "ASTC_12x12");
#endif

        if (rd->m_hwTexFormatSupport.m_FormatBC1.IsValid() || rd->m_hwTexFormatSupport.m_FormatBC2.IsValid() || rd->m_hwTexFormatSupport.m_FormatBC3.IsValid())
        {
            rd->m_Features |= RFT_COMPRESSTEXTURE;
        }
    }

    rd->m_Features |= RFT_HW_HDR;

    rd->m_nHDRType = 1;

    rd->m_FullResRect.right = rd->m_width;
    rd->m_FullResRect.bottom = rd->m_height;

#if defined(WIN32) || defined(WIN64) || defined(APPLE) || defined(LINUX) || defined(CREATE_DEVICE_ON_MAIN_THREAD)
    rd->m_pRT->RC_SetViewport(0, 0, rd->m_width, rd->m_height);
#else
    rd->RT_SetViewport(0, 0, rd->m_width, rd->m_height);
#endif
    rd->m_MainViewport.nX = 0;
    rd->m_MainViewport.nY = 0;
    rd->m_MainViewport.nWidth = rd->m_width;
    rd->m_MainViewport.nHeight = rd->m_height;

    return S_OK;
}

HRESULT CALLBACK CD3D9Renderer::OnD3D11PostCreateDevice(D3DDevice* pd3dDevice)
{
    LOADING_TIME_PROFILE_SECTION;
    CD3D9Renderer* rd = gcpRendD3D;
    HRESULT hr;

#if defined(SUPPORT_DEVICE_INFO)
    rd->BindContextToThread(CryGetCurrentThreadId());
    rd->m_DeviceContext = rd->m_devInfo.Context();

    rd->m_pBackBuffer = rd->m_devInfo.BackbufferRTV();
    rd->m_pBackBuffers = rd->m_devInfo.BackbufferRTVs();
    rd->m_pSwapChain = rd->m_devInfo.SwapChain();
    rd->m_pCurrentBackBufferIndex = rd->GetCurrentBackBufferIndex(rd->m_pSwapChain);
#endif

    void* pBackBuf;
    hr = rd->m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuf);
    if (FAILED(hr))
    {
        return hr;
    }
    ID3D11Texture2D* pBackBuffer = (ID3D11Texture2D*)pBackBuf;
    D3D11_TEXTURE2D_DESC backBufferSurfaceDesc;
    pBackBuffer->GetDesc(&backBufferSurfaceDesc);
    ZeroMemory(&rd->m_d3dsdBackBuffer, sizeof(DXGI_SURFACE_DESC));
    rd->m_d3dsdBackBuffer.Width = (UINT) backBufferSurfaceDesc.Width;
    rd->m_d3dsdBackBuffer.Height = (UINT) backBufferSurfaceDesc.Height;
#if defined(SUPPORT_DEVICE_INFO)
    rd->m_d3dsdBackBuffer.Format = backBufferSurfaceDesc.Format;
    rd->m_d3dsdBackBuffer.SampleDesc = backBufferSurfaceDesc.SampleDesc;
    rd->m_ZFormat = rd->m_devInfo.AutoDepthStencilFmt();
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_14
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
    SAFE_RELEASE(pBackBuffer);

    if (FAILED(hr))
    {
        return hr;
    }

    // Collect depth stencil parameters
    D3D11_TEXTURE2D_DESC dsTextureDesc;
    dsTextureDesc.MipLevels = 1;
    dsTextureDesc.ArraySize = 1;
    dsTextureDesc.Format = rd->m_ZFormat;
    dsTextureDesc.SampleDesc = rd->m_d3dsdBackBuffer.SampleDesc;
    dsTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    dsTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    dsTextureDesc.CPUAccessFlags = 0;
    dsTextureDesc.MiscFlags = 0;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc;
    dsViewDesc.Format = CTexture::ConvertToDepthStencilFmt(dsTextureDesc.Format);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_15
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    dsViewDesc.Flags = 0;
#endif
    dsViewDesc.ViewDimension = dsTextureDesc.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
    dsViewDesc.Texture2D.MipSlice = 0;

    const float clearDepth = CRenderer::CV_r_ReverseDepth ? 0.f : 1.f;
    const uint clearStencil = 1;
    const FLOAT clearValues[4] = { clearDepth, FLOAT(clearStencil) };

    int nDepthBufferWidth = rd->IsEditorMode() ? rd->m_d3dsdBackBuffer.Width : rd->m_width;
    int nDepthBufferHeight = rd->IsEditorMode() ? rd->m_d3dsdBackBuffer.Height : rd->m_height;

    // Create the depth stencil buffer for scene rendering
    SAFE_RELEASE(rd->m_pZTexture);
    SAFE_RELEASE(rd->m_pZBuffer);
    dsTextureDesc.Width  = nDepthBufferWidth;
    dsTextureDesc.Height = nDepthBufferHeight;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_16
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
    hr = rd->m_DevMan.CreateD3D11Texture2D(&dsTextureDesc, clearValues, NULL, &rd->m_pZTexture, "DepthBuffer");
    if (FAILED(hr))
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_17
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
        return hr;
    }
    hr = rd->GetDevice().CreateDepthStencilView(rd->m_pZTexture, &dsViewDesc, &rd->m_pZBuffer);
    if (FAILED(hr))
    {
        return hr;
    }
    rd->GetDeviceContext().ClearDepthStencilView(rd->m_pZBuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);

    // Create the native resolution depth stencil buffer for overlay rendering if needed
    SAFE_RELEASE(rd->m_pNativeZTexture);
    SAFE_RELEASE(rd->m_pNativeZBuffer);
    if (!rd->IsEditorMode() && (gcpRendD3D->GetOverlayWidth() != dsTextureDesc.Width || gcpRendD3D->GetOverlayHeight() != dsTextureDesc.Height))
    {
        dsTextureDesc.Width  = gcpRendD3D->GetOverlayWidth();
        dsTextureDesc.Height = gcpRendD3D->GetOverlayHeight();
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_18
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
        hr = rd->m_DevMan.CreateD3D11Texture2D(&dsTextureDesc, clearValues, NULL, &rd->m_pNativeZTexture, "DepthBuffer");
        if (FAILED(hr))
        {
            return hr;
        }
        hr = rd->GetDevice().CreateDepthStencilView(rd->m_pNativeZTexture, &dsViewDesc, &rd->m_pNativeZBuffer);
        if (FAILED(hr))
        {
            return hr;
        }
        rd->GetDeviceContext().ClearDepthStencilView(rd->m_pNativeZBuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
    }
    else
    {
        rd->m_pNativeZTexture = rd->m_pZTexture;
        rd->m_pNativeZBuffer = rd->m_pZBuffer;
        rd->m_pNativeZTexture->AddRef();
        rd->m_pNativeZBuffer->AddRef();
    }
    rd->m_DepthBufferOrig.pTex = nullptr;
    rd->m_DepthBufferOrig.pTarget = rd->m_pZTexture;
    rd->m_DepthBufferOrig.pSurf = rd->m_pZBuffer;
    rd->m_pZBuffer->AddRef();
    
    rd->m_DepthBufferOrigMSAA.pTex = nullptr;
    rd->m_DepthBufferOrigMSAA.pTarget = rd->m_pZTexture;
    rd->m_DepthBufferOrigMSAA.pSurf = rd->m_pZBuffer;
    rd->m_pZBuffer->AddRef();
    
    rd->m_DepthBufferNative.pTex = nullptr;
    rd->m_DepthBufferNative.pTarget = rd->m_pNativeZTexture;
    rd->m_DepthBufferNative.pSurf = rd->m_pNativeZBuffer;
    rd->m_pNativeZBuffer->AddRef();

    rd->m_nRTStackLevel[0] = 0;
    if (rd->m_d3dsdBackBuffer.Width == rd->m_nativeWidth && rd->m_d3dsdBackBuffer.Height == rd->m_nativeHeight)
    {
        rd->m_RTStack[0][0].m_pDepth = rd->m_pNativeZBuffer;
        rd->m_RTStack[0][0].m_pSurfDepth = &rd->m_DepthBufferNative;
    }
    else
    {
        rd->m_RTStack[0][0].m_pDepth = NULL;
        rd->m_RTStack[0][0].m_pSurfDepth = NULL;
    }
    rd->m_RTStack[0][0].m_pTarget = rd->m_pBackBuffer;
    rd->m_RTStack[0][0].m_Width = rd->m_d3dsdBackBuffer.Width;
    rd->m_RTStack[0][0].m_Height = rd->m_d3dsdBackBuffer.Height;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_19
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    rd->m_RTStack[0][0].m_bScreenVP = false;
#endif
    rd->m_RTStack[0][0].m_bWasSetRT = false;
    rd->m_RTStack[0][0].m_bWasSetD = false;
    rd->m_nMaxRT2Commit = 0;
    rd->m_pNewTarget[0] = &rd->m_RTStack[0][0];
    rd->FX_SetActiveRenderTargets();

    for (int i = 0; i < RT_STACK_WIDTH; i++)
    {
        rd->m_pNewTarget[i] = &rd->m_RTStack[i][0];
        rd->m_pCurTarget[i] = rd->m_pNewTarget[0]->m_pTex;
    }

    rd->m_DepthBufferOrig.nWidth = nDepthBufferWidth;
    rd->m_DepthBufferOrig.nHeight = nDepthBufferHeight;
    rd->m_DepthBufferOrig.bBusy = true;
    rd->m_DepthBufferOrig.nFrameAccess = -2;

    rd->m_DepthBufferOrigMSAA.nWidth = nDepthBufferWidth;
    rd->m_DepthBufferOrigMSAA.nHeight = nDepthBufferHeight;
    rd->m_DepthBufferOrigMSAA.bBusy = true;
    rd->m_DepthBufferOrigMSAA.nFrameAccess = -2;

    rd->m_DepthBufferNative.nWidth = rd->m_nativeWidth;
    rd->m_DepthBufferNative.nHeight = rd->m_nativeHeight;
    rd->m_DepthBufferNative.bBusy = true;
    rd->m_DepthBufferNative.nFrameAccess = -2;

    SAFE_RELEASE(rd->m_RP.m_MSAAData.m_pDepthTex);
    SAFE_RELEASE(rd->m_RP.m_MSAAData.m_pZBuffer);

    rd->CreateMSAADepthBuffer();

    // Create shader bindable depthstencil buffer view and shader resource view. Ideally would be unified into regular texture creation - requires big refactoring
    SAFE_RELEASE(rd->m_pZBufferReadOnlyDSV);
    SAFE_RELEASE(rd->m_pZBufferDepthReadOnlySRV);
    SAFE_RELEASE(rd->m_pZBufferStencilReadOnlySRV);
    D3DTexture* pDepthStencil = rd->m_DepthBufferOrigMSAA.pTarget;
    D3D11_TEXTURE2D_DESC descDepthStencil;
    pDepthStencil->GetDesc(&descDepthStencil);

#if defined(SUPPORT_DEVICE_INFO)
    if (rd->DevInfo().FeatureLevel() >= D3D_FEATURE_LEVEL_11_0) // Read-only depth-stencil supported on 11.0 feature level and above, leave NULL otherwise resulting in no testing
#endif //defined(SUPPORT_DEVICE_INFO)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
        ZeroStruct(descDSV);
        descDSV.Format = CTexture::ConvertToDepthStencilFmt(descDepthStencil.Format);
        descDSV.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
        descDSV.ViewDimension = (descDepthStencil.SampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
        descDSV.Texture2D.MipSlice = 0;
        hr = rd->GetDevice().CreateDepthStencilView(pDepthStencil, &descDSV, &rd->m_pZBufferReadOnlyDSV);
        assert(SUCCEEDED(hr));
    }

    D3DFormat nDepthStencilFormatTypeless = descDepthStencil.Format;
    if (!CTexture::IsDeviceFormatTypeless(nDepthStencilFormatTypeless))
    {
        nDepthStencilFormatTypeless = CTexture::ConvertToTypelessFmt(nDepthStencilFormatTypeless);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    ZeroStruct(SRVDesc);
    SRVDesc.Format = CTexture::ConvertToShaderResourceFmt(nDepthStencilFormatTypeless);
    SRVDesc.ViewDimension = (descDepthStencil.SampleDesc.Count > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MipLevels = 1;
    hr = rd->GetDevice().CreateShaderResourceView(pDepthStencil, &SRVDesc, &rd->m_pZBufferDepthReadOnlySRV);
    assert(SUCCEEDED(hr));

    if (RenderCapabilities::SupportsStencilTextures())
    {
        SRVDesc.Format = CTexture::ConvertToStencilFmt(nDepthStencilFormatTypeless);
        hr = rd->GetDevice().CreateShaderResourceView(pDepthStencil, &SRVDesc, &rd->m_pZBufferStencilReadOnlySRV);
        assert(SUCCEEDED(hr));
    }

#if !defined(_RELEASE)
#if defined(WIN32)
#define D3DSYSTEM_CPP_USE_PRIVATEDATA
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_20
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#endif

#if defined(D3DSYSTEM_CPP_USE_PRIVATEDATA)
    if (rd->m_pZTexture)
    {
        AZStd::string dsvName("$MainDepthStencil");
        rd->m_pZTexture->SetPrivateData(WKPDID_D3DDebugObjectName, dsvName.length(), dsvName.c_str());
    }

    if (rd->m_pZBuffer)
    {
        AZStd::string srvName("[DSV] $MainDepthStencil");
        rd->m_pZBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, srvName.length(), srvName.c_str());
    }

    if (rd->m_pZBufferReadOnlyDSV)
    {
        AZStd::string srvName("[DSV] $MainDepthStencil - Read Only");
        rd->m_pZBufferReadOnlyDSV->SetPrivateData(WKPDID_D3DDebugObjectName, srvName.length(), srvName.c_str());
    }

    if (rd->m_pZBufferDepthReadOnlySRV)
    {
        AZStd::string srvName("[SRV] $MainDepthStencil - Depth Read Only");
        rd->m_pZBufferDepthReadOnlySRV->SetPrivateData(WKPDID_D3DDebugObjectName, srvName.length(), srvName.c_str());
    }

    if (rd->m_pZBufferStencilReadOnlySRV)
    {
        AZStd::string srvName("[SRV] $MainDepthStencil - Stencil Read Only");
        rd->m_pZBufferStencilReadOnlySRV->SetPrivateData(WKPDID_D3DDebugObjectName, srvName.length(), srvName.c_str());
    }
#endif

    rd->ReleaseAuxiliaryMeshes();
    rd->CreateAuxiliaryMeshes();

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DSYSTEM_CPP_SECTION_21
    #include AZ_RESTRICTED_FILE(D3DSystem_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    D3D11_QUERY_DESC QDesc;
    QDesc.Query = D3D11_QUERY_EVENT;
    QDesc.MiscFlags = 0;
    for (int i = 0; i < 2; ++i)
    {
        hr = pd3dDevice->CreateQuery(&QDesc, &rd->m_pQuery[i]);
        assert(hr == S_OK && rd->m_pQuery[i]);
        rd->GetDeviceContext().End(rd->m_pQuery[i]);
    }
#endif
    rd->EF_Restore();

    rd->m_bDeviceLost = 0;
    rd->m_pLastVDeclaration = NULL;

#if defined(ENABLE_RENDER_AUX_GEOM)
    if (rd->m_pRenderAuxGeomD3D && FAILED(hr = rd->m_pRenderAuxGeomD3D->RestoreDeviceObjects()))
    {
        return hr;
    }
#endif

    CHWShader_D3D::mfSetGlobalParams();

    if (rd->m_OcclQueries.capacity())
    {
        for (int a = 0; a < MAX_OCCL_QUERIES; a++)
        {
            rd->m_OcclQueries[a].Release();
        }
    }

    {
        LOADING_TIME_PROFILE_SECTION_NAMED("CD3D9Renderer::OnD3D10PostCreateDevice(): m_OcclQueries");
        rd->m_OcclQueries.Reserve(MAX_OCCL_QUERIES);
        // Lazy initialization on Android due to limited number of queries that we can create.
        // TODO Linux - This was crashing on Ubuntu, investigate
#if !defined(AZ_PLATFORM_ANDROID) && !defined(AZ_PLATFORM_LINUX)
        for (int a = 0; a < MAX_OCCL_QUERIES; a++)
        {
            rd->m_OcclQueries[a].Create();
        }
#endif
    }


    return S_OK;
}

#if defined(WIN32)
// Renderer looks for multi-monitor setup changes and fullscreen key combination
bool CD3D9Renderer::HandleMessage([[maybe_unused]] HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, [[maybe_unused]] LRESULT* pResult)
{
    switch (message)
    {
    case WM_DISPLAYCHANGE:
    case WM_DEVICECHANGE:
    {
        // Count the number of connected display devices
        bool bHaveMonitorsChanged = true;
        uint connectedMonitors = 0;
        EnumDisplayMonitors(NULL, NULL, CountConnectedMonitors, reinterpret_cast<LPARAM>(&connectedMonitors));

        // Check for changes
        if (connectedMonitors > m_nConnectedMonitors)
        {
            iSystem->GetILog()->LogAlways("[Renderer] A display device has been connected to the system");
        }
        else if (connectedMonitors < m_nConnectedMonitors)
        {
            iSystem->GetILog()->LogAlways("[Renderer] A display device has been disconnected from the system");
        }
        else
        {
            bHaveMonitorsChanged = false;
        }

        // Update state
        m_nConnectedMonitors = connectedMonitors;
        m_bDisplayChanged = bHaveMonitorsChanged;
    }
    break;

    case WM_SYSKEYDOWN:
    {
        const bool bAlt = (lParam & (1 << 29)) != 0;
        if (wParam == VK_RETURN && bAlt)     // ALT+ENTER
        {
            ICVar* pVar = iConsole->GetCVar("r_fullscreen");
            if (pVar)
            {
                int fullscreen = pVar->GetIVal();
                pVar->Set((int)(fullscreen == 0));     // Toggle CVar
            }
        }
    }
    break;
    }
    return false;
}
#endif // defined(WIN32)

#if defined(SUPPORT_DEVICE_INFO_USER_DISPLAY_OVERRIDES)
static const char* GetScanlineOrderNaming(DXGI_MODE_SCANLINE_ORDER v)
{
    switch (v)
    {
    case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE:
        return "progressive";
    case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST:
        return "interlaced (upper field first)";
    case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST:
        return "interlaced (lower field first)";
    default:
        return "unspecified";
    }
}

void UserOverrideDisplayProperties(DXGI_MODE_DESC& desc)
{
    if (gRenDev->m_CVFullScreen->GetIVal())
    {
        if (gRenDev->CV_r_overrideRefreshRate > 0)
        {
            DXGI_RATIONAL& refreshRate = desc.RefreshRate;
            if (refreshRate.Denominator)
            {
                gEnv->pLog->Log("Overriding refresh rate to %.2f Hz (was %.2f Hz).", (float)gRenDev->CV_r_overrideRefreshRate, (float)refreshRate.Numerator / (float)refreshRate.Denominator);
            }
            else
            {
                gEnv->pLog->Log("Overriding refresh rate to %.2f Hz (was undefined).", (float)gRenDev->CV_r_overrideRefreshRate);
            }
            refreshRate.Numerator = (unsigned int) (gRenDev->CV_r_overrideRefreshRate * 1000.0f);
            refreshRate.Denominator = 1000;
        }

        if (gRenDev->CV_r_overrideScanlineOrder > 0)
        {
            DXGI_MODE_SCANLINE_ORDER old = desc.ScanlineOrdering;
            DXGI_MODE_SCANLINE_ORDER& so = desc.ScanlineOrdering;
            switch (gRenDev->CV_r_overrideScanlineOrder)
            {
            case 2:
                so = DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST;
                break;
            case 3:
                so = DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST;
                break;
            default:
                so = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
                break;
            }
            gEnv->pLog->Log("Overriding scanline order to %s (was %s).", GetScanlineOrderNaming(so), GetScanlineOrderNaming(old));
        }
    }
}
#endif


#include "DeviceInfo.inl"


void EnableCloseButton([[maybe_unused]] void* hWnd, [[maybe_unused]] bool enabled)
{
#if defined(WIN32) || defined(WIN64)
    if (hWnd)
    {
        HMENU hMenu = GetSystemMenu((HWND) hWnd, FALSE);
        if (hMenu)
        {
            const unsigned int flags = enabled ? MF_ENABLED : (MF_DISABLED | MF_GRAYED);
            EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | flags);
        }
    }
#endif
}

#if defined(SUPPORT_D3D_DEBUG_RUNTIME)
string D3DDebug_GetLastMessage()
{
    return gcpRendD3D->m_d3dDebug.GetLastMessage();
}
#endif
