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

#include "PerforcePlugin_precompiled.h"
#include "PerforcePlugin.h"
#include "PerforceSourceControl.h"
#include "Include/ISourceControl.h"
#include "Include/IEditorClassFactory.h"

extern CPerforceSourceControl* g_pPerforceControl;

namespace PluginInfo
{
    const char* kName = "Perforce Client";
    const char* kGUID = "{FD5F1023-8F02-4051-89FA-DF1F038863A2}";
    const int kVersion = 1;
}

void CPerforcePlugin::Release()
{
    GetIEditor()->GetClassFactory()->UnregisterClass(g_pPerforceControl->ClassName().toUtf8().data());
    delete this;
}

void CPerforcePlugin::ShowAbout()
{
}

const char* CPerforcePlugin::GetPluginGUID()
{
    return PluginInfo::kGUID;
}

DWORD CPerforcePlugin::GetPluginVersion()
{
    return PluginInfo::kVersion;
}

const char* CPerforcePlugin::GetPluginName()
{
    return PluginInfo::kName;
}

bool CPerforcePlugin::CanExitNow()
{
    return true;
}

void CPerforcePlugin::Serialize([[maybe_unused]] FILE* hFile, [[maybe_unused]] bool bIsStoring)
{
}

void CPerforcePlugin::ResetContent()
{
}

bool CPerforcePlugin::CreateUIElements()
{
    return true;
}

void CPerforcePlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
    if (eNotify_OnInit == aEventId)
    {
        g_pPerforceControl->Init();
    }
}
