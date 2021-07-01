/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
