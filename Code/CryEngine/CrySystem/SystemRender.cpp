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
