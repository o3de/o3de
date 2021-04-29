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

// Description : CryENGINE system core


#include "CrySystem_precompiled.h"
#include "System.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#if defined(AZ_PLATFORM_IOS)
#import <UIKit/UIKit.h>
#endif 

#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <IProcess.h>
#include "Log.h"
#include "XConsole.h"
#include <I3DEngine.h>
#include <CryLibrary.h>
#include "PhysRenderer.h"
#include <IMovieSystem.h>

#include "CrySizerStats.h"
#include "CrySizerImpl.h"
#include "VisRegTest.h"
#include "ITextModeConsole.h"
#include <ILevelSystem.h>
#include <LyShine/ILyShine.h>

#include "MiniGUI/MiniGUI.h"
#include "PerfHUD.h"
#include "ThreadInfo.h"

#include <LoadScreenBus.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SYSTEMRENDERER_CPP_SECTION_1 1
#define SYSTEMRENDERER_CPP_SECTION_2 2
#endif

extern CMTSafeHeap* g_pPakHeap;
#if defined(AZ_PLATFORM_ANDROID)
#include <AzCore/Android/Utils.h>
#elif defined(AZ_PLATFORM_IOS)

#if defined(AZ_MONOLITHIC_BUILD)
//extern bool UIKitGetPrimaryPhysicalDisplayDimensions(int& o_widthPixels, int& o_heightPixels);
#else

using NativeScreenType = UIScreen;
using NativeWindowType = UIWindow;

////////////////////////////////////////////////////////////////////////////////
// bool UIKitGetPrimaryPhysicalDisplayDimensions(int& o_widthPixels, int& o_heightPixels)
// {

//     NativeScreenType* nativeScreen = [NativeScreenType mainScreen];
//     CGRect screenBounds = [nativeScreen bounds];
//     CGFloat screenScale = [nativeScreen scale];
//     o_widthPixels = static_cast<int>(screenBounds.size.width * screenScale);
//     o_heightPixels = static_cast<int>(screenBounds.size.height * screenScale);

//     const bool isScreenLandscape = o_widthPixels > o_heightPixels;

//     UIInterfaceOrientation uiOrientation = UIInterfaceOrientationUnknown;
// #if defined(__IPHONE_13_0) || defined(__TVOS_13_0)
//     if(@available(iOS 13.0, tvOS 13.0, *))
//     {
//         UIWindow* foundWindow = nil;
        
//         //Find the key window
//         NSArray* windows = [[UIApplication sharedApplication] windows];
//         for (UIWindow* window in windows)
//         {
//             if (window.isKeyWindow)
//             {
//                 foundWindow = window;
//                 break;
//             }
//         }
        
//         //Check if the key window is found
//         if(foundWindow)
//         {
//             uiOrientation = foundWindow.windowScene.interfaceOrientation;
//         }
//         else
//         {
//             //If no key window is found create a temporary window in order to extract the orientation
//             //This can happen as this function gets called before the renderer is initialized
//             CGRect screenBounds = [[NativeScreenType mainScreen] bounds];
//             UIWindow* tempWindow = [[NativeWindowType alloc] initWithFrame: screenBounds];
//             uiOrientation = tempWindow.windowScene.interfaceOrientation;
//             [tempWindow release];
//         }
//     }
// #else
//     uiOrientation = UIApplication.sharedApplication.statusBarOrientation;
// #endif
    
//     const bool isInterfaceLandscape = UIInterfaceOrientationIsLandscape(uiOrientation);
//     if (isScreenLandscape != isInterfaceLandscape)
//     {
//         const int width = o_widthPixels;
//         o_widthPixels = o_heightPixels;
//         o_heightPixels = width;
//     }

//     return true;
// }
#endif

#endif

extern int CryMemoryGetAllocatedSize();

/////////////////////////////////////////////////////////////////////////////////
static void VerifySizeRenderVar(ICVar* pVar)
{
    const int size = pVar->GetIVal();
    if (size <= 0)
    {
        AZ_Error("Console Variable", false, "'%s' set to invalid value: %i. Setting to nearest safe value: 1.", pVar->GetName(), size);
        pVar->Set(1);
    }
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::GetPrimaryPhysicalDisplayDimensions([[maybe_unused]] int& o_widthPixels, [[maybe_unused]] int& o_heightPixels)
{
#if defined(AZ_PLATFORM_WINDOWS)
    o_widthPixels = GetSystemMetrics(SM_CXSCREEN);
    o_heightPixels = GetSystemMetrics(SM_CYSCREEN);
    return true;
#elif defined(AZ_PLATFORM_ANDROID)
    return AZ::Android::Utils::GetWindowSize(o_widthPixels, o_heightPixels);
// #elif defined(AZ_PLATFORM_IOS)
//     return UIKitGetPrimaryPhysicalDisplayDimensions(o_widthPixels, o_heightPixels); // RedCode, no longer needed with Atom
#else
    return false;
#endif
}


bool CSystem::IsTablet()
{
//TODO: Add support for Android tablets
#if defined(AZ_PLATFORM_IOS)
    return [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad;
#else
    return false;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
void CSystem::CreateRendererVars(const SSystemInitParams& startupParams)
{
    int iFullScreenDefault = 1;
    int iDisplayInfoDefault = 0;
    int iWidthDefault = 1280;
    int iHeightDefault = 720;
#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_IOS)
    GetPrimaryPhysicalDisplayDimensions(iWidthDefault, iHeightDefault);
#elif defined(WIN32) || defined(WIN64)
    iFullScreenDefault = 0;
    iWidthDefault = GetSystemMetrics(SM_CXFULLSCREEN) * 2 / 3;
    iHeightDefault = GetSystemMetrics(SM_CYFULLSCREEN) * 2 / 3;
#endif

    if (IsDevMode())
    {
        iFullScreenDefault = 0;
        iDisplayInfoDefault = 1;
    }

    // load renderer settings from engine.ini
    m_rWidth = REGISTER_INT_CB("r_Width", iWidthDefault, VF_DUMPTODISK,
            "Sets the display width, in pixels. Default is 1280.\n"
            "Usage: r_Width [800/1024/..]", VerifySizeRenderVar);
    m_rHeight = REGISTER_INT_CB("r_Height", iHeightDefault, VF_DUMPTODISK,
            "Sets the display height, in pixels. Default is 720.\n"
            "Usage: r_Height [600/768/..]", VerifySizeRenderVar);
    m_rWidthAndHeightAsFractionOfScreenSize = REGISTER_FLOAT("r_WidthAndHeightAsFractionOfScreenSize", 1.0f, VF_DUMPTODISK,
            "(iOS/Android only) Sets the display width and height as a fraction of the physical screen size. Default is 1.0.\n"
            "Usage: rWidthAndHeightAsFractionOfScreenSize [0.1 - 1.0]");
    m_rTabletWidthAndHeightAsFractionOfScreenSize = REGISTER_FLOAT("r_TabletWidthAndHeightAsFractionOfScreenSize", 1.0f, VF_DUMPTODISK,
            "(iOS only) NOTE: TABLETS ONLY Sets the display width and height as a fraction of the physical screen size. Default is 1.0.\n"
            "Usage: rTabletWidthAndHeightAsFractionOfScreenSize [0.1 - 1.0]");
    m_rMaxWidth = REGISTER_INT("r_MaxWidth", 0, VF_DUMPTODISK,
            "(iOS/Android only) Sets the maximum display width while maintaining the device aspect ratio.\n"
            "Usage: r_MaxWidth [1024/1920/..] (0 for no max), combined with r_WidthAndHeightAsFractionOfScreenSize [0.1 - 1.0]");
    m_rMaxHeight = REGISTER_INT("r_MaxHeight", 0, VF_DUMPTODISK,
            "(iOS/Android only) Sets the maximum display height while maintaining the device aspect ratio.\n"
            "Usage: r_MaxHeight [768/1080/..] (0 for no max), combined with r_WidthAndHeightAsFractionOfScreenSize [0.1 - 1.0]");
    m_rColorBits = REGISTER_INT("r_ColorBits", 32, VF_DUMPTODISK,
            "Sets the color resolution, in bits per pixel. Default is 32.\n"
            "Usage: r_ColorBits [32/24/16/8]");
    m_rDepthBits = REGISTER_INT("r_DepthBits", 24, VF_DUMPTODISK | VF_REQUIRE_APP_RESTART,
            "Sets the depth precision, in bits per pixel. Default is 24.\n"
            "Usage: r_DepthBits [32/24]");
    m_rStencilBits = REGISTER_INT("r_StencilBits", 8, VF_DUMPTODISK,
            "Sets the stencil precision, in bits per pixel. Default is 8.\n");

    // Needs to be initialized as soon as possible due to swap chain creation modifications..
    m_rHDRDolby = REGISTER_INT_CB("r_HDRDolby", 0, VF_DUMPTODISK,
            "HDR dolby output mode\n"
            "Usage: r_HDRDolby [Value]\n"
            "0: Off (default)\n"
            "1: Dolby maui output\n"
            "2: Dolby vision output\n",
            [] (ICVar* cvar)
            {
                eDolbyVisionMode mode = static_cast<eDolbyVisionMode>(cvar->GetIVal());

                if (mode == eDVM_Vision && gEnv->IsEditor())
                {
                    cvar->Set(static_cast<int>(eDVM_Disabled));
                }
            });
    // Restrict the limits of this cvar to the eDolbyVisionMode values
    m_rHDRDolby->SetLimits(static_cast<float>(eDVM_Disabled), static_cast<float>(eDVM_Vision));

#if defined(WIN32) || defined(WIN64)
    REGISTER_INT("r_overrideDXGIAdapter", -1, VF_REQUIRE_APP_RESTART,
        "Specifies index of the preferred video adapter to be used for rendering (-1=off, loops until first suitable adapter is found).\n"
        "Use this to resolve which video card to use if more than one DX11 capable GPU is available in the system.");
#endif

#if defined(WIN32) || defined(WIN64)
    const char* p_r_DriverDef = "Auto";
#elif defined(APPLE)
    const char* p_r_DriverDef = "METAL";
#elif defined(ANDROID)
    const char* p_r_DriverDef = "GL";
#elif defined(LINUX)
    const char* p_r_DriverDef = "GL";
    if (gEnv->IsDedicated())
    {
        p_r_DriverDef = "NULL";
    }
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMRENDERER_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(SystemRender_cpp)
#else
    const char* p_r_DriverDef = "DX9";                          // required to be deactivated for final release
#endif
    if (startupParams.pCvarsDefault)
    { // hack to customize the default value of r_Driver
        SCvarsDefault* pCvarsDefault = startupParams.pCvarsDefault;
        if (pCvarsDefault->sz_r_DriverDef && pCvarsDefault->sz_r_DriverDef[0])
        {
            p_r_DriverDef = startupParams.pCvarsDefault->sz_r_DriverDef;
        }
    }

    m_rDriver = REGISTER_STRING("r_Driver", p_r_DriverDef, VF_DUMPTODISK | VF_INVISIBLE,
            "Sets the renderer driver ( DX11/AUTO/NULL ).\n"
            "Specify in bootstrap.cfg like this: r_Driver = \"DX11\"");

    m_rFullscreen = REGISTER_INT("r_Fullscreen", iFullScreenDefault, VF_DUMPTODISK,
            "Toggles fullscreen mode. Default is 1 in normal game and 0 in DevMode.\n"
            "Usage: r_Fullscreen [0=window/1=fullscreen]");

    m_rFullscreenWindow = REGISTER_INT("r_FullscreenWindow", 0, VF_DUMPTODISK,
            "Toggles fullscreen-as-window mode. Fills screen but allows seamless switching. Default is 0.\n"
            "Usage: r_FullscreenWindow [0=locked fullscreen/1=fullscreen as window]");

    m_rFullscreenNativeRes = REGISTER_INT("r_FullscreenNativeRes", 0, VF_DUMPTODISK, "");

    m_rDisplayInfo = REGISTER_INT("r_DisplayInfo", iDisplayInfoDefault, VF_RESTRICTEDMODE | VF_DUMPTODISK,
            "Toggles debugging information display.\n"
            "Usage: r_DisplayInfo [0=off/1=show/2=enhanced/3=compact]");

    m_rOverscanBordersDrawDebugView = REGISTER_INT("r_OverscanBordersDrawDebugView", 0, VF_RESTRICTEDMODE | VF_DUMPTODISK,
            "Toggles drawing overscan borders.\n"
            "Usage: r_OverscanBordersDrawDebugView [0=off/1=show]");
}

//////////////////////////////////////////////////////////////////////////
void CSystem::RenderBegin()
{
    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

    if (m_bIgnoreUpdates)
    {
        return;
    }

    bool rndAvail = m_env.pRenderer != 0;

    //////////////////////////////////////////////////////////////////////
    //start the rendering pipeline
    if (rndAvail)
    {
        m_env.pRenderer->BeginFrame();
    }

    gEnv->nMainFrameID = (rndAvail) ? m_env.pRenderer->GetFrameID(false) : 0;
}


//////////////////////////////////////////////////////////////////////////
void CSystem::RenderEnd([[maybe_unused]] bool bRenderStats, bool bMainWindow)
{
    {
        FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

        if (m_bIgnoreUpdates)
        {
            return;
        }

        if (!m_env.pRenderer)
        {
            return;
        }

        if (bMainWindow)    // we don't do this in UI Editor window for example
        {
#if !defined (_RELEASE)
            // Flush render data and swap buffers.
            m_env.pRenderer->RenderDebug(bRenderStats);
#endif

#if defined(USE_PERFHUD)
            if (m_pPerfHUD)
            {
                m_pPerfHUD->Draw();
            }
            if (m_pMiniGUI)
            {
                m_pMiniGUI->Draw();
            }
#endif

            if (!gEnv->pSystem->GetILevelSystem() || !gEnv->pSystem->GetILevelSystem()->IsLevelLoaded())
            {
                IConsole* console = GetIConsole();

                //Same goes for the console. When no level is loaded, it's okay to render it outside of the renderer
                //so that users can load maps or change settings.
                if (console != nullptr)
                {
                    console->Draw();
                }
            }
        }

        m_env.pRenderer->ForceGC(); // XXX Rename this
        m_env.pRenderer->EndFrame();


        if (IConsole* pConsole = GetIConsole())
        {
            // if we have pending cvar calculation, execute it here
            // since we know cvars will be correct here after ->EndFrame().
            if (!pConsole->IsHashCalculated())
            {
                pConsole->CalcCheatVarHash();
            }
        }
    }
}

void CSystem::OnScene3DEnd()
{
    //Render Console
    if (m_bDrawConsole && gEnv->pConsole)
    {
        gEnv->pConsole->Draw();
    }
}


//! Update screen and call some important tick functions during loading.
void CSystem::SynchronousLoadingTick([[maybe_unused]] const char* pFunc, [[maybe_unused]] int line)
{
    LOADING_TIME_PROFILE_SECTION;
    if (gEnv && gEnv->bMultiplayer && !gEnv->IsEditor())
    {
        //UpdateLoadingScreen currently contains a couple of tick functions that need to be called regularly during the synchronous level loading,
        //when the usual engine and game ticks are suspended.
        UpdateLoadingScreen();

#if defined(MAP_LOADING_SLICING)
        GetISystemScheduler()->SliceAndSleep(pFunc, line);
#endif
    }
}


//////////////////////////////////////////////////////////////////////////
void CSystem::UpdateLoadingScreen()
{
    // Do not update the network thread from here - it will cause context corruption - use the NetworkStallTicker thread system

    if (GetCurrentThreadId() != gEnv->mMainThreadId)
    {
        return;
    }

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMRENDERER_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(SystemRender_cpp)
#endif

#if AZ_LOADSCREENCOMPONENT_ENABLED
    EBUS_EVENT(LoadScreenBus, UpdateAndRender);
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

    if (!m_bEditor && !IsQuitting())
    {
        if (m_pProgressListener)
        {
            m_pProgressListener->OnLoadingProgress(0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CSystem::DisplayErrorMessage(const char* acMessage,
    [[maybe_unused]] float fTime,
    const float* pfColor,
    bool bHardError)
{
    SErrorMessage message;
    message.m_Message = acMessage;
    if (pfColor)
    {
        memcpy(message.m_Color, pfColor, 4 * sizeof(float));
    }
    else
    {
        message.m_Color[0] = 1.0f;
        message.m_Color[1] = 0.0f;
        message.m_Color[2] = 0.0f;
        message.m_Color[3] = 1.0f;
    }
    message.m_HardFailure = bHardError;
#ifdef _RELEASE
    message.m_fTimeToShow = fTime;
#else
    message.m_fTimeToShow = 1.0f;
#endif
    m_ErrorMessages.push_back(message);
}

//! Renders the statistics; this is called from RenderEnd, but if the
//! Host application (Editor) doesn't employ the Render cycle in ISystem,
//! it may call this method to render the essential statistics
//////////////////////////////////////////////////////////////////////////
void CSystem::RenderStatistics()
{
    RenderStats();
}

//////////////////////////////////////////////////////////////////////
void CSystem::Render()
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    //check what is the current process
    if (!m_pProcess)
    {
        return; //should never happen
    }
    //check if the game is in pause or
    //in menu mode
    //bool bPause=false;
    //if (m_pProcess->GetFlags() & PROC_MENU)
    //  bPause=true;

    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

    //////////////////////////////////////////////////////////////////////
    //draw
    m_env.p3DEngine->PreWorldStreamUpdate(m_ViewCamera);

    if (m_pProcess)
    {
        if (m_pProcess->GetFlags() & PROC_3DENGINE)
        {
            if (!gEnv->IsEditing())  // Editor calls it's own rendering update
            {
                if (m_env.p3DEngine && !m_env.IsFMVPlaying())
                {
                    if (!IsEquivalent(m_ViewCamera.GetPosition(), Vec3(0, 0, 0), VEC_EPSILON) || // never pass undefined camera to p3DEngine->RenderWorld()
                        gEnv->IsDedicated() || (gEnv->pRenderer && gEnv->pRenderer->IsPost3DRendererEnabled()))
                    {
                        GetIRenderer()->SetViewport(0, 0, GetIRenderer()->GetWidth(), GetIRenderer()->GetHeight());
                        m_env.p3DEngine->RenderWorld(SHDF_ALLOW_WATER | SHDF_ALLOWPOSTPROCESS | SHDF_ALLOWHDR | SHDF_ZPASS | SHDF_ALLOW_AO, SRenderingPassInfo::CreateGeneralPassRenderingInfo(m_ViewCamera), __FUNCTION__);
                    }
                    else
                    {
                        if (gEnv->pRenderer)
                        {
                            // force rendering of black screen to be sure we don't only render the clear color (which is the fog color by default)
                            gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
                            gEnv->pRenderer->Draw2dImage(0, 0, 800, 600, -1, 0.0f, 0.0f, 1.0f, 1.0f,   0.f,
                                0.0f, 0.0f, 0.0f, 1.0f, 0.f);
                        }
                    }
                }
#if !defined(_RELEASE)
                if (m_pVisRegTest)
                {
                    m_pVisRegTest->AfterRender();
                }
#endif

                if (m_env.pMovieSystem)
                {
                    m_env.pMovieSystem->Render();
                }
            }
        }
        else
        {
            GetIRenderer()->SetViewport(0, 0, GetIRenderer()->GetWidth(), GetIRenderer()->GetHeight());
            m_pProcess->RenderWorld(SHDF_ALLOW_WATER | SHDF_ALLOWPOSTPROCESS | SHDF_ALLOWHDR | SHDF_ZPASS | SHDF_ALLOW_AO, SRenderingPassInfo::CreateGeneralPassRenderingInfo(m_ViewCamera), __FUNCTION__);
        }
    }

    m_env.p3DEngine->WorldStreamUpdate();

    gEnv->pRenderer->SwitchToNativeResolutionBackbuffer();
}


//////////////////////////////////////////////////////////////////////////
void CSystem::RenderStats()
{
#if defined(ENABLE_PROFILING_CODE)
#ifndef _RELEASE
    // if we rendered an error message on screen during the last frame, then sleep now
    // to force hard stall for 3sec
    if (m_bHasRenderedErrorMessage && !gEnv->IsEditor() && !IsLoading())
    {
        // DO NOT REMOVE OR COMMENT THIS OUT!
        // If you hit this, then you most likely have invalid (synchronous) file accesses
        // which must be fixed in order to not stall the entire game.
        Sleep(3000);
        m_bHasRenderedErrorMessage = false;
    }
#endif

    // render info messages on screen
    float fTextPosX = 5.0f;
    float fTextPosY = -10;
    float fTextStepY = 13;

    float fFrameTime = gEnv->pTimer->GetRealFrameTime();
    TErrorMessages::iterator itnext;
    for (TErrorMessages::iterator it = m_ErrorMessages.begin(); it != m_ErrorMessages.end(); it = itnext)
    {
        itnext = it;
        ++itnext;
        SErrorMessage& message = *it;

        SDrawTextInfo ti;
        ti.flags = eDrawText_FixedSize | eDrawText_2D | eDrawText_Monospace;
        memcpy(ti.color, message.m_Color, 4 * sizeof(float));
        ti.xscale = ti.yscale = 1.4f;
        m_env.pRenderer->DrawTextQueued(Vec3(fTextPosX, fTextPosY += fTextStepY, 1.0f), ti, message.m_Message.c_str());

        if (!IsLoading())
        {
            message.m_fTimeToShow -= fFrameTime;
        }

        if (message.m_HardFailure)
        {
            m_bHasRenderedErrorMessage = true;
        }

        if (message.m_fTimeToShow < 0.0f)
        {
            m_ErrorMessages.erase(it);
        }
    }
#endif

    if (!m_env.pConsole)
    {
        return;
    }

#ifndef _RELEASE
    if (m_rOverscanBordersDrawDebugView)
    {
        RenderOverscanBorders();
    }
#endif

    int iDisplayInfo = m_rDisplayInfo->GetIVal();
    if (iDisplayInfo == 0)
    {
        return;
    }

    // Draw engine stats
    if (m_env.p3DEngine)
    {
        // Draw 3dengine stats and get last text cursor position
        float nTextPosX = 101 - 20, nTextPosY = -2, nTextStepY = 3;
        m_env.p3DEngine->DisplayInfo(nTextPosX, nTextPosY, nTextStepY, iDisplayInfo != 1);

        // Dump Open 3D Engine CPU and GPU memory statistics to screen
        m_env.p3DEngine->DisplayMemoryStatistics();

    #if defined(ENABLE_LW_PROFILERS)
        if (m_rDisplayInfo->GetIVal() == 2)
        {
            m_env.pRenderer->TextToScreen(nTextPosX, nTextPosY += nTextStepY, "SysMem %.1f mb",
                float(DumpMMStats(false)) / 1024.f);
        }
    #endif

    #if 0
        for (int i = 0; i < NUM_POOLS; ++i)
        {
            int used = (g_pPakHeap->m_iBigPoolUsed[i] ? (int)g_pPakHeap->m_iBigPoolSize[i] : 0);
            int size = (int)g_pPakHeap->m_iBigPoolSize[i];
            float fC1[4] = {1, 1, 0, 1};
            m_env.pRenderer->Draw2dLabel(10, 100.0f + i * 16, 2.1f, fC1, false,  "BigPool %d: %d bytes of %d bytes used", i, used, size);
        }
    #endif
    }
}

void CSystem::RenderOverscanBorders()
{
#ifndef _RELEASE

    if (m_env.pRenderer && m_rOverscanBordersDrawDebugView)
    {
        int iOverscanBordersDrawDebugView = m_rOverscanBordersDrawDebugView->GetIVal();
        if (iOverscanBordersDrawDebugView)
        {
            const int texId = -1;
            const float uv = 0.0f;
            const float rot = 0.0f;
            const int whiteTextureId = m_env.pRenderer->GetWhiteTextureId();

            const float r = 1.0f;
            const float g = 1.0f;
            const float b = 1.0f;
            const float a = 0.2f;

            Vec2 overscanBorders = Vec2(0.0f, 0.0f);
            m_env.pRenderer->EF_Query(EFQ_OverscanBorders, overscanBorders);

            const float overscanBorderWidth = overscanBorders.x * VIRTUAL_SCREEN_WIDTH;
            const float overscanBorderHeight = overscanBorders.y * VIRTUAL_SCREEN_HEIGHT;

            const float xPos = overscanBorderWidth;
            const float yPos = overscanBorderHeight;
            const float width = VIRTUAL_SCREEN_WIDTH - (2.0f * overscanBorderWidth);
            const float height = VIRTUAL_SCREEN_HEIGHT - (2.0f * overscanBorderHeight);

            m_env.pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
            m_env.pRenderer->Draw2dImage(xPos, yPos,
                width, height,
                whiteTextureId,
                uv, uv, uv, uv,
                rot,
                r, g, b, a);
        }
    }
#endif
}
