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

#include <LoadScreenBus.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SYSTEMRENDERER_CPP_SECTION_1 1
#define SYSTEMRENDERER_CPP_SECTION_2 2
#endif

#if defined(AZ_PLATFORM_ANDROID)
#include <AzCore/Android/Utils.h>
#endif

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
