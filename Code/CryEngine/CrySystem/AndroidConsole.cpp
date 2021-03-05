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

// Description : Console implementation for Android, reports back to the main interface.


#include "CrySystem_precompiled.h"
#if defined(ANDROID)
#include "AndroidConsole.h"

#include "android/log.h"
CAndroidConsole::CAndroidConsole()
    : m_isInitialized(false)
{
}

CAndroidConsole::~CAndroidConsole()
{
}

// Interface IOutputPrintSink /////////////////////////////////////////////
void CAndroidConsole::Print(const char* line)
{
    __android_log_print(ANDROID_LOG_VERBOSE, "CryEngine", "MSG: %s\n", line);
}
// Interface ISystemUserCallback //////////////////////////////////////////
bool CAndroidConsole::OnError(const char* errorString)
{
    __android_log_print(ANDROID_LOG_ERROR, "CryEngine", "ERR: %s\n", errorString);
    return true;
}

void CAndroidConsole::OnInitProgress(const char* sProgressMsg)
{
    (void) sProgressMsg;
    // Do Nothing
}
void CAndroidConsole::OnInit(ISystem* pSystem)
{
    if (!m_isInitialized)
    {
        IConsole* pConsole = pSystem->GetIConsole();
        if (pConsole != 0)
        {
            pConsole->AddOutputPrintSink(this);
        }
        m_isInitialized = true;
    }
}
void CAndroidConsole::OnShutdown()
{
    if (m_isInitialized)
    {
        // remove outputprintsink
        m_isInitialized = false;
    }
}
void CAndroidConsole::OnUpdate()
{
    // Do Nothing
}
void CAndroidConsole::GetMemoryUsage(ICrySizer* pSizer)
{
    size_t size = sizeof(*this);



    pSizer->AddObject(this, size);
}

// Interface ITextModeConsole /////////////////////////////////////////////
Vec2_tpl<int> CAndroidConsole::BeginDraw()
{
    return Vec2_tpl<int>(0, 0);
}
void CAndroidConsole::PutText(int x, int y, const char* msg)
{
    __android_log_print(ANDROID_LOG_VERBOSE, "CryEngine", "PUT: %s\n", msg);
}
void CAndroidConsole::EndDraw()
{
    // Do Nothing
}
#endif // ANDROID
