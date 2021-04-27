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

#include "CrySystem_precompiled.h"
#if defined(IOS)
#include "IOSConsole.h"



CIOSConsole::CIOSConsole():
m_isInitialized(false)
{
    
}

CIOSConsole::~CIOSConsole()
{
    
}

// Interface IOutputPrintSink /////////////////////////////////////////////
void CIOSConsole::Print(const char *line)
{
    printf("MSG: %s\n", line);
}
// Interface ISystemUserCallback //////////////////////////////////////////
bool CIOSConsole::OnError(const char *errorString)
{
    printf("ERR: %s\n", errorString);
    return true;
}

void CIOSConsole::OnInitProgress(const char *sProgressMsg)
{
    (void) sProgressMsg;
    // Do Nothing
}
void CIOSConsole::OnInit(ISystem *pSystem)
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
void CIOSConsole::OnShutdown()
{
    if (m_isInitialized)
    {
        // remove outputprintsink
        m_isInitialized = false;
    }
}
void CIOSConsole::OnUpdate()
{
    // Do Nothing
}
void CIOSConsole::GetMemoryUsage(ICrySizer *pSizer)
{
    size_t size = sizeof(*this);
    
    
    
    pSizer->AddObject(this, size);
}

// Interface ITextModeConsole /////////////////////////////////////////////
Vec2_tpl<int> CIOSConsole::BeginDraw()
{
    return Vec2_tpl<int>(0,0);
}
void CIOSConsole::PutText( int x, int y, const char * msg )
{
    printf("PUT: %s\n", msg);
}
void CIOSConsole::EndDraw() {
    // Do Nothing
}
#endif // IOS
