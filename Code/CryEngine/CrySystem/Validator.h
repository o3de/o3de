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

#ifndef CRYINCLUDE_CRYSYSTEM_VALIDATOR_H
#define CRYINCLUDE_CRYSYSTEM_VALIDATOR_H

#pragma once

//////////////////////////////////////////////////////////////////////////
// Default validator implementation.
//////////////////////////////////////////////////////////////////////////
struct SDefaultValidator
    : public IValidator
{
    CSystem* m_pSystem;
    SDefaultValidator(CSystem* system)
        : m_pSystem(system) {};
    virtual void Report(SValidatorRecord& record)
    {
        if (record.text)
        {
            static bool bNoMsgBoxOnWarnings = false;
            if ((record.text[0] == '!')  || (m_pSystem->m_sysWarnings && m_pSystem->m_sysWarnings->GetIVal() != 0))
            {
                if (g_cvars.sys_no_crash_dialog)
                {
                    return;
                }

                if (bNoMsgBoxOnWarnings)
                {
                    return;
                }

#ifdef WIN32
                ICVar* pFullscreen = (gEnv && gEnv->pConsole) ? gEnv->pConsole->GetCVar("r_Fullscreen") : 0;
                if (pFullscreen && pFullscreen->GetIVal() != 0 && gEnv->pRenderer && gEnv->pRenderer->GetHWND())
                {
                    ::ShowWindow((HWND)gEnv->pRenderer->GetHWND(), SW_MINIMIZE);
                }
                string strMessage = record.text;
                strMessage += "\n---------------------------------------------\nAbort - terminate application\nRetry - continue running the application\nIgnore - don't show this message box any more";
                switch (::MessageBox(NULL, strMessage.c_str(), "CryEngine Warning", MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 | MB_ICONWARNING | MB_SYSTEMMODAL))
                {
                case IDABORT:
                    m_pSystem->GetIConsole()->Exit ("User abort requested during showing the warning box with the following message: %s", record.text);
                    break;
                case IDRETRY:
                    break;
                case IDIGNORE:
                    bNoMsgBoxOnWarnings = true;
                    m_pSystem->m_sysWarnings->Set(0);
                    break;
                }
#endif
            }
        }
    }
};

#endif // CRYINCLUDE_CRYSYSTEM_VALIDATOR_H
