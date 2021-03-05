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

#include "FBXPlugin_precompiled.h"
#include "FBXPlugin.h"

namespace PluginInfo
{
    const char* kName = "FBX Exporter";
    const char* kGUID = "{6CD02F95-362C-4ADF-8BAE-87C6342A8027}";
    const int kVersion = 1;
}

void CFBXPlugin::Release()
{
    delete this;
}

void CFBXPlugin::ShowAbout()
{
}

const char* CFBXPlugin::GetPluginGUID()
{
    return PluginInfo::kGUID;
}

DWORD CFBXPlugin::GetPluginVersion()
{
    return PluginInfo::kVersion;
}

const char* CFBXPlugin::GetPluginName()
{
    return PluginInfo::kName;
}

bool CFBXPlugin::CanExitNow()
{
    return true;
}

void CFBXPlugin::Serialize([[maybe_unused]] FILE* hFile, [[maybe_unused]] bool bIsStoring)
{
}

void CFBXPlugin::ResetContent()
{
}

bool CFBXPlugin::CreateUIElements()
{
    return true;
}

void CFBXPlugin::OnEditorNotify([[maybe_unused]] EEditorNotifyEvent aEventId)
{
}
