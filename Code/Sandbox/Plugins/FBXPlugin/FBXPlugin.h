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

#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012
// -------------------------------------------------------------------------
//  Created:     24 June 2011 by Sergiy Shaykin.
//  Description: Attachment to the Sandbox plug-in system
//
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_FBXPLUGIN_FBXPLUGIN_H
#define CRYINCLUDE_FBXPLUGIN_FBXPLUGIN_H
#include <Include/IPlugin.h>

class CFBXPlugin
    : public IPlugin
{
public:
    void Release();
    void ShowAbout();
    const char* GetPluginGUID();
    DWORD GetPluginVersion();
    const char* GetPluginName();
    bool CanExitNow();
    void Serialize(FILE* hFile, bool bIsStoring);
    void ResetContent();
    bool CreateUIElements();
    void OnEditorNotify(EEditorNotifyEvent aEventId);
};
#endif // CRYINCLUDE_FBXPLUGIN_FBXPLUGIN_H
