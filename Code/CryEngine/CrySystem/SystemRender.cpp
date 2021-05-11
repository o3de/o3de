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

#include "ITextModeConsole.h"
#include <ILevelSystem.h>
#include <LyShine/ILyShine.h>

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
